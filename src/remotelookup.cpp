/**
 *  RemoteLookup.cpp
 *  
 *  Implementation file for the RemoteLookup class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "remotelookup.h"
#include "connection.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/answer.h"
#include "../include/dnscpp/handler.h"
#include "../include/dnscpp/question.h"
#include "fakeresponse.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  core        dns core object
 *  @param  domain      the domain of the lookup
 *  @param  type        the type of the request
 *  @param  bits        bits to include
 *  @param  handler     user space object
 */
RemoteLookup::RemoteLookup(Core *core, const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler) : 
    Operation(handler, ns_o_query, domain, type, bits), _core(core)
{
    // call "retry" to send the first datagram to the first nameserver
    if (!_core->nameservers().empty()) retry(_started);
    
    // if there are no nameservers, we set a timer to expire immediately
    else _timer = _core->loop()->timer(0.0, this);
}

/**
 *  Destructor
 */
RemoteLookup::~RemoteLookup()
{
    // no need to cleanup if the job was already over
    if (_timer == nullptr) return;
    
    // cleanup the job (note that we have this cleanup-function because we
    // normally want to cleanup _before_ we report back to userspace, because
    // you never know what userspace will do (maybe even destruct the _core pointer),
    // but if userspace decided to kill the job (by calling job->cancel()) we still
    // have to do some cleaning ourselves
    cleanup();

    // if the operation is destructed while the timer was still running, it means that the
    // operation was prematurely cancelled from user-space, let the handler know
    _handler->onCancelled(this);
}

/**
 *  How long should we wait until the next message?
 *  @param  now         Current time
 *  @return double
 */
double RemoteLookup::delay(double now) const
{
    // the number of servers that we have
    size_t servers = _core->nameservers().size();
    
    // have we already completed a full round of all nameservers? if that is not yet
    // the case we send the next message soon, with a small delay to spread out the messages
    if (_count % servers != 0) return _core->spread();
    
    // a full round has been completed, how many rounds have we had?
    size_t rounds = _count / servers;
    
    // calculate the time when to start round + 1
    double nexttime = _started + rounds * _core->interval();
    
    // calculate delay based on this time
    return std::max(std::min(nexttime, expires()) - now, 0.0);
}

/**
 *  Cleanup the object
 *  We want to cleanup the job _before_ it is destructed, to handle the situation
 *  where user-space already destructs _core while the job is reporting its result
 */
void RemoteLookup::cleanup()
{
    // forget the tcp connection
    _connection.reset();
    
    // unsubscribe from the nameservers
    for (auto &nameserver : _core->nameservers()) nameserver.unsubscribe(this, _query.id());
    
    // stop the timer
    if (_timer) _core->loop()->cancel(_timer, this);
    
    // forget the timer
    _timer = nullptr;
}

/**
 *  When does the job expire?
 *  @return double
 */
double RemoteLookup::expires() const
{
    // if there are no nameservers, the call expires immediately (waiting is pointless)
    if (_core->nameservers().empty()) return _started;
    
    // get the max time
    return _started + _core->expire();
}

/** 
 *  Time out the job because no appropriate response was received in time
 *  @param  busy        is dns-cpp still busy processing buffers?
 */
void RemoteLookup::timeout(bool busy)
{
    // if we have to wait until all buffers are processed
    if (busy) return;
    
    // before we report to userspace we cleanup the object
    cleanup();
    
    // report an error
    _handler->onTimeout(this);
    
    // self-destruct
    delete this;
}

/**
 *  Retry / send a new message to one of the nameservers
 *  @param  now     current timestamp
 */
void RemoteLookup::retry(double now)
{
    // we need some "random" identity if the rotate option is set because we do not want all jobs to start with 
    // nameserver[0] -- for this we use the starttime as it is random-enough to distribute requests. otherwise 
    // we will always start at nameserver 0
    size_t id = _core->rotate() ? _started * 100000 : 0;
    
    // access to the nameservers + the number we have
    auto &nameservers = _core->nameservers();
    size_t nscount = nameservers.size();
    
    // which nameserver should we sent now?
    size_t target = (_count + id) % nscount;
    
    // iterator for the next loop
    size_t i = 0;
    
    // send a datagram to each nameserver
    for (auto &nameserver : nameservers)
    {
        // is this the target nameserver? (we use ++ postfix operator on purpose)
        if (target != i++) continue;
        
        // send a datagram to this server
        nameserver.datagram(_query);
        
        // in the first iteration we have not yet subscribed
        if (_count < nscount) nameserver.subscribe(this, _query.id());

        // one more message has been sent
        _count += 1;
        
        // for now we do not yet send the next message
        break;
    }
    
    // we set a new timer for when the entire job times out
    _timer = _core->loop()->timer(delay(now), this);
}

/**
 *  When the timer expires
 *  This method is called from the event loop in user space
 */
void RemoteLookup::expire()
{
    // cancel (deallocate) the timer
    _core->loop()->cancel(_timer, this);
    
    // the timer has expired
    _timer = nullptr;

    // find the current time
    Now now;
    
    // did the entire job expire?
    if (now >= expires()) return timeout(_core->readable());
    
    // if we do not yet have a tcp connection we send out more dgrams
    if (!_connection) return retry(now);
    
    // we set a new timer for when the entire job times out
    _timer = _core->loop()->timer(expires() - now, this);
}

/**
 *  Method to report the response
 *  This method checks if there is an NXDOMAIN error, if that is the case
 *  it is turned into an empty response if the /etc/hosts file holds a record for the host
 *  @param  response
 */
void RemoteLookup::report(const Response &response)
{
    // for NXDOMAIN errors we need special treatment (maybe the hostname _does_ exists in 
    // /etc/hosts?) For all other type of results the message can be passed to userspace
    if (response.rcode() != ns_r_nxdomain) return _handler->onReceived(this, response);

    // extract the original question, to find out the host for which we were looking
    Question question(response);
    
    // there was a NXDOMAIN error, which we should not communicate if our /etc/hosts
    // file does have a record for this hostname, check this
    if (!_core->exists(question.name())) return _handler->onReceived(this, response);
    
    // get the original request (so that the response can match the request)
    Request request(this);
    
    // construct a fake-response message (it is fake because we have not actually received it)
    FakeResponse fake(request, question);

    // send the fake-response to user-space
    _handler->onReceived(this, Response(fake.data(), fake.size()));
}

/**
 *  Method that is called when a response is received
 *  @param  nameserver  the reporting nameserver
 *  @param  response    the received response
 *  @return bool        was the response processed?
 */
bool RemoteLookup::onReceived(Nameserver *nameserver, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return false;
    
    // if we're already busy with a tcp connection we ignore further dgram responses
    if (_connection) return false;
    
    // if the response was truncated, we ignore it and start a tcp connection
    if (response.truncated()) return _connection.reset(new Connection(_core->loop(), nameserver->ip(), _query, response, this)), false;
    
    // before we report to userspace we cleanup the object
    cleanup();

    // we have a response, so we can pass that to user space
    report(response);
    
    // we can self-destruct -- this job has been handled
    delete this;
    
    // job has been handled
    return true;
}

/**
 *  Method that is called when the nameserver is no longer expecting any responses
 *  Because of an optimization deeper inside Udp.cpp, this is only called when ALL nameservers are idle
 *  @param  nameserver  the reporting nameserver
 */
void RemoteLookup::onIdle(Nameserver *nameserver)
{
    // find the current time
    Now now;
    
    // all nameservers are idle, if this request expired in the meantime, we can get rid of it
    if (now >= expires()) return timeout(false);
}

/**
 *  Called when the response has been received
 *  @param  connection
 *  @param  response
 */
void RemoteLookup::onReceived(Connection *connection, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return;

    // before we report to userspace we cleanup the object
    cleanup();
    
    // we have a response, hand it over to user space
    report(response);
    
    // self-destruct now that the job has been completed
    delete this;
}

/**
 *  Called when the connection could not be used
 *  @param  connector   the reporting connection
 *  @param  response    the original answer (the original truncated one)
 */
void RemoteLookup::onFailure(Connection *connection, const Response &truncated)
{
    // before we report to userspace we cleanup the object
    cleanup();

    // we failed to get the regular response, so we send back the truncated response
    _handler->onReceived(this, truncated);

    // self-destruct now that the job has been completed
    delete this;
}

/**
 *  End of namespace
 */
}

