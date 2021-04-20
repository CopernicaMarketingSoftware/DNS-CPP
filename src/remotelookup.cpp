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
    Lookup(handler, ns_o_query, domain, type, bits), _core(core), _id(rand()) {}

/**
 *  Destructor
 */
RemoteLookup::~RemoteLookup()
{
    // cleanup the job (note that we have this cleanup-function because we
    // normally want to cleanup _before_ we report back to userspace, because
    // you never know what userspace will do (maybe even destruct the _core pointer),
    // but if userspace decided to kill the job (by calling job->cancel()) we still
    // have to do some cleaning ourselves
    cleanup();
}

/**
 *  Is this lookup still scheduled: meaning that no requests has been sent yet
 *  @return bool
 */
bool RemoteLookup::scheduled() const
{
    // if the operation is still busy, but has not sent out anything
    return _handler != nullptr && _count == 0;
}

/**
 *  Is this lookup already finished: meaning that a result has been reported back to userspace
 *  @return bool
 */
bool RemoteLookup::finished() const
{
    // handler is reset on completion, so that can be used for checking
    return _handler == nullptr;
}

/**
 *  Is this lookup exhausted: meaning that it has sent its max number of requests, but still
 *  has not received an appropriate answer, and is now waiting for its final timer to finish
 *  @return bool
 */
bool RemoteLookup::exhausted() const
{
    // handler must still exist (result has not been reported)
    if (_handler == nullptr) return false;
    
    // if a tcp connection is in progress to solve a truncated response we are not going to send out more datagrams
    if (_truncated) return true;
    
    // if max number of datagrams has not yet been reached
    return _count >= _core->attempts();
}

/**
 *  How long should we wait until the next message?
 *  @param  now         Current time
 *  @return double
 */
double RemoteLookup::delay(double now) const
{
    // if the operation is ready, we should run asap (so that it is removed)
    // if the operation never ran it should also run immediately
    if (_count == 0 || _handler == nullptr) return 0.0;
    
    // if already doing a tcp lookup, or when all attemps have passed, we wait until the expire-time
    if (_truncated || _count >= _core->attempts()) return std::max(0.0, _last + _core->timeout() - now);
    
    // wait until we can send a next datagram
    return std::max(_last + _core->interval() - now, 0.0);
}

/**
 *  Unsubscribe from all inbound UDP sockets
 */
void RemoteLookup::unsubscribe()
{
    // unsubscribe from the UDP sockets
    for (const auto &subscription : _subscriptions)
    {
        // this is a pair
        subscription.first->unsubscribe(this, subscription.second, _query.id());
    }

    // we have no subscriptions left
    _subscriptions.clear();
}    

/**
 *  Cleanup the object
 *  We want to cleanup the job _before_ it is destructed, to handle the situation
 *  where user-space already destructs _core while the job is reporting its result
 *  @return Handler     the handler that may still be called
 */
Handler *RemoteLookup::cleanup()
{
    // remember the old handler
    auto handler = _handler;
    
    // forget the handler
    _handler = nullptr;
    
    // unsubscribe from all inbound sockets
    unsubscribe();

    // expose the handler
    return handler;
}

/** 
 *  Time out the job because no appropriate response was received in time
 *  @return bool        wsa there a call to userspace?
 */
bool RemoteLookup::timeout()
{
    // before we report to userspace we cleanup the object
    cleanup()->onTimeout(this);
    
    // done
    return true;
}

/**
 *  Execute the lookup. Returns true when a user-space call was made, and false when further
 *  processing is required.
 *  @param  now         current time
 *  @return bool        was there a call back to userspace?
 */
bool RemoteLookup::execute(double now)
{
    // when job times out
    if ((_truncated || _count >= _core->attempts()) && now > _last + _core->timeout()) return timeout();

    // if we reached the max attempts we stop sending out more datagrams
    if (_count >= _core->attempts()) return false;
    
    // if the operation is already using tcp we simply wait for that
    if (_truncated) return false;

    // access to the nameservers + the number we have
    auto &nameservers = _core->nameservers();
    size_t nscount = nameservers.size();
    
    // what if there are no nameservers?
    if (nscount == 0) return timeout();

    // which nameserver should we sent now?
    size_t target = _core->rotate() ? (_count + _id) % nscount : _count % nscount;
    
    // send a datagram to each nameserver
    auto &nameserver = nameservers[target];

    // send a datagram to this server
    auto *inbound = _core->datagram(nameserver, _query);

    // one more message has been sent
    _count += 1; _last = now;
    
    // if the datagram was not _really_ sent (unlikely), we will treat it just as if it WAS sent,
    // so that the problem will be picked up when the timer expires
    if (inbound == nullptr) return false;
    
    // subscribe to the answers that might come in from now onwards
    inbound->subscribe(this, nameserver, _query.id());
    
    // store this subscription, so that we can unsubscribe on success
    _subscriptions.emplace(std::make_pair(inbound, nameserver));

    // no call to user space
    return false;
}

/**
 *  Method to report the response
 *  This method checks if there is an NXDOMAIN error, if that is the case
 *  it is turned into an empty response if the /etc/hosts file holds a record for the host
 *  @param  response        the response to report
 *  @return bool            was there a call to userspace?
 */
bool RemoteLookup::report(const Response &response)
{
    // if the result has already been reported, we do nothing here
    if (_handler == nullptr) return false;
    
    // for NXDOMAIN errors we need special treatment (maybe the hostname _does_ exists in 
    // /etc/hosts?) For all other type of results the message can be passed to userspace
    if (response.rcode() != ns_r_nxdomain) return cleanup()->onReceived(this, response), true;

    // extract the original question, to find out the host for which we were looking
    Question question(response);
    
    // there was a NXDOMAIN error, which we should not communicate if our /etc/hosts
    // file does have a record for this hostname, check this
    if (!_core->exists(question.name())) return cleanup()->onReceived(this, response), true;
    
    // get the original request (so that the response can match the request)
    Request request(this);
    
    // construct a fake-response message (it is fake because we have not actually received it)
    FakeResponse fake(request, question);

    // send the fake-response to user-space
    cleanup()->onReceived(this, Response(fake.data(), fake.size()));
    
    // done
    return true;
}

/**
 *  Method that is called when a response is received
 *  @param  nameserver  the reporting nameserver
 *  @param  response    the received response
 *  @return bool        was the response processed / was there a call to user space?
 */
bool RemoteLookup::onReceived(const Ip &ip, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return false;
    
    // if the response was not truncated, we can report it to userspace, we do this also
    // when the response came from a TCP lookup and was still truncated (_truncated is used as a boolean to indicate tcp)
    if (!response.truncated() || _truncated) return report(response);

    // we can unsubscribe from all inbound udp sockets because we're no longer interested in those responses
    unsubscribe();
    
    // try to connect to a TCP socket
    if (!_core->connect(ip, this)) return report(response);
    
    // We remember the truncated response in case tcp fails too, so that we at least have _something_ to 
    // report in case TCP is unavailable. Note that the default user-space onReceived() handler turns truncated 
    // responses into onFailure()-calls, so in most user space applications a truncation-plus-failed-tcp 
    // ends up as a SERVFAIL anyway. However, user space programs that want to distinguish a SERVFAIL-rcode 
    // from a truncation error can still write their own onReceived() method because of this:
    _truncated.reset(new Response(response));

    // remember the start-time of the connection to reset the timeout-period
    _last = Now();
    
    // done (return false because there was no call to userspace yet)
    return false;
}

/**
 *  Called when a TCP connection has been set up (in case an earlier UDP response was truncated)
 *  @param  ip          ip to which a connection was set up
 *  @param  tcp         the actual TCP connection
 *  @return bool        was a call to userspace done?
 */
bool RemoteLookup::onConnected(const Ip &ip, Tcp *tcp)
{
    // send the query
    auto *inbound = tcp->send(_query);
        
    // subscribe to the answers that might come in from now onwards
    inbound->subscribe(this, ip, _query.id());
        
    // store this subscription, so that we can unsubscribe on success
    _subscriptions.emplace(std::make_pair(inbound, ip));
    
    // no call to userspace yet
    return false;
}

/**
 *  Called when a TCP connection could not be set up
 *  @param  ip          ip to which a connection was set up
 *  @return bool        was a call to userspace done?
 */
bool RemoteLookup::onFailure(const Ip &ip)
{
    // tcp failed, in this case we want to send the truncated response instead
    return report(*_truncated);
}

/**
 *  Cancel the operation
 */
void RemoteLookup::cancel()
{
    // do nothing if already cancelled
    if (_handler == nullptr) return;
    
    // cleanup, and remove to userspace
    cleanup()->onCancelled(this);
}

/**
 *  End of namespace
 */
}

