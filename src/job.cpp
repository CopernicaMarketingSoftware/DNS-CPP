/**
 *  Job.cpp
 *  
 *  Implementation file for the Job class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "job.h"
#include "connection.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/answer.h"
#include "../include/dnscpp/handler.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  core        dns core object
 *  @param  domain      the domain of the lookup
 *  @param  type        the type of the request
 *  @param  handler     user space object
 */
Job::Job(Core *core, const char *domain, ns_type type, DNS::Handler *handler) : 
    Operation(handler, ns_o_query, domain, type, core->dnssec()), _core(core)
{
    // iterate over the nameservers to subscribe to incoming responses
    for (auto &nameserver : core->nameservers()) nameserver.subscribe(this);
    
    // "retry" sending a datagram
    retry(_started);
}

/**
 *  How long should we wait until the next message?
 *  @param  now         Current time
 *  @return double
 */
double Job::delay(double now) const
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
void Job::cleanup()
{
    // unsubscribe from the nameservers
    for (auto &nameserver : _core->nameservers()) nameserver.unsubscribe(this);
    
    // stop the timer
    if (_timer) _core->loop()->cancel(_timer, this);
    
    // forget the timer
    _timer = nullptr;
}

/**
 *  When does the job expire?
 *  @return double
 */
double Job::expires() const
{
    // get the max time
    return _started + _core->expire();
}

/** 
 *  Time out the job because no appropriate response was received in time
 */
void Job::timeout()
{
    // before we report to userspace we cleanup the object
    cleanup();
    
    // report an error
    _handler->onTimeout(this);
    
    // self-destruct
    delete this;
}

/**
 *  Retry / send a new message to the nameservers
 *  @param  now     current timestamp
 */
void Job::retry(double now)
{
    // we need some "random" identity because we do not want all jobs to start with 
    // nameserver[0] -- for this we use the starttime as it is random-enough to distribute requests
    size_t id = _started * 100000;
    
    // which nameserver should we sent now?
    size_t target = (_count + id) % _core->nameservers().size();
    
    // iterator for the next loop
    size_t i = 0;
    
    // send a datagram to each nameserver
    for (auto &nameserver : _core->nameservers())
    {
        // is this the target nameserver? (we use ++ postfix operator on purpose)
        if (target != i++) continue;
        
        // send a datagram to this server
        nameserver.datagram(_query);

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
void Job::expire()
{
    // find the current time
    Now now;
    
    // cancel (deallocate) the timer
    _core->loop()->cancel(_timer, this);
    
    // the timer has expired
    _timer = nullptr;
    
    // did the entire job expire?
    if (now >= expires()) return timeout();
    
    // if we do not yet have a tcp connection we send out more dgrams
    if (!_connection) return retry(now);
    
    // we set a new timer for when the entire job times out
    _timer = _core->loop()->timer(expires() - now, this);
}

/**
 *  Method that is called when a response is received
 *  @param  nameserver  the reporting nameserver
 *  @param  response    the received response
 */
void Job::onReceived(Nameserver *nameserver, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return;
    
    // if we're already busy with a tcp connection we ignore further dgram responses
    if (_connection) return;
    
    // if the response was truncated, we ignore it and start a tcp connection
    if (response.truncated()) return _connection.reset(new Connection(_core->loop(), nameserver->ip(), _query, response, this));
    
    // before we report to userspace we cleanup the object
    cleanup();

    // we have a response, so we can pass that to user space
    _handler->onReceived(this, response);
    
    // we can self-destruct -- this job has been handled
    delete this;
}

/**
 *  Called when the response has been received
 *  @param  connection
 *  @param  response
 */
void Job::onReceived(Connection *connection, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return;

    // before we report to userspace we cleanup the object
    cleanup();
    
    // we have a response, hand it over to user space
    _handler->onReceived(this, response);
    
    // self-destruct now that the job has been completed
    delete this;
}

/**
 *  Called when the connection could not be used
 *  @param  connector   the reporting connection
 *  @param  response    the original answer (the original truncated one)
 */
void Job::onFailure(Connection *connection, const Response &truncated)
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

