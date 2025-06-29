/**
 *  Core.cpp
 * 
 *  Implementation file for the Core class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2025 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/lookup.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/watcher.h"
#include "idgenerator.h"
#include "searchlookup.h"
#include "locallookup.h"
#include "remotelookup.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        your event loop
 *  @throws std::runtime_error
 */
Core::Core(Loop *loop) :
    _loop(loop),
    _ipv4(loop, this),
    _ipv6(loop, this) {}

/**
 *  Destructor
 */
Core::~Core()
{
    // stop timer (in case it is still running)
    if (_timer == nullptr) return;
    
    // stop the timer
    _loop->cancel(_timer, this);
}

/**
 *  Set the capacity: number of operations to run at the same time
 *  @param  value       the new value
 */
void Core::capacity(size_t value)
{
    // store property
    _capacity = std::min((size_t)IdGenerator::capacity(), std::max(size_t(1), value));
}

/**
 *  Add a new lookup to the list
 *  @param  lookup
 *  @return Operation
 */
Operation *Core::add(Lookup *lookup)
{
    // WARNING: purists (like me) will not like this method because we make some assumptions about 
    // whether `lookup` is a RemoteLookup or LocalLookup plus how those lookups are implemented. 
    // Technically, this is not necessary (we could have written agnostic code too), but this allows 
    // us to make things more efficient (for example by immediately calling execute() without first 
    // going back to the event-loop because we KNOW that execute() will not trigger a user space call)
    
    // in case the lookup is already exhausted (no more udp messages have to be sent), we can
    // put it in the ready-queue right away (in reality this means that this is a LocalLookup)
    if (lookup->exhausted())
    {
        // IN THEORY we should call delay(), and put it either on the front or the end of the queue
        // based on that -- but in this case we know that this code is only executed for local-lookups 
        // and the delay for those lookups is hardcoded to 0.0 anyway
        _ready.emplace_front(lookup);
        
        // we consider this lookup to be active (note that we might exceed _capacity now, but we're
        // ok with that as this is a local-lookup that does not put any stress on nameservers)
        _inflight += 1;
        
        // expire soon so that it is picked up and reported to user space
        timer(0.0);
    }
    else if (_capacity <= _inflight || lookup->config()->nameservers() == 0)
    {
        // we are going to update _scheduled, check if this is the first time
        bool wasempty = _scheduled.empty();
        
        // this is a remote-lookup, but we have too many operations already in progress so we
        // delay sending out the first datagram (or, unlikely, there are no nameservers configured, 
        // meaning that a call to lookup::execute() would trigger a call to userspace, which we want 
        // to avoid to keep all callbacks ASYNC, so we also want to delay the call to execute())
        _scheduled.emplace_back(lookup);
        
        // make sure the timer expires in time
        if (wasempty) reschedule(Now{});
    }
    else
    {
        // we need the current time
        Now now;
        
        // THEORETICALLY, we should not immediately call execute() because that might trigger a
        // call to user-space (while user-space expects ASYNC callbacks). However, since this code
        // is in REALITY only used for RemoteLookups and we know for sure that there are nameservers
        // (so the call to execute() will not trigger a call to user-space) we execute() right
        // away for reasons of efficiency.
        lookup->execute(now);
        
        // the operation is in progress
        _inflight += 1;
        
        // we are going to update _lookups, check if this was the first time
        bool wasempty = _lookups.empty();
        
        // put it at the back of the queue to schedule it for repeating the call
        _lookups.emplace_back(lookup);
        
        // we might have to set a timer
        if (wasempty) reschedule(now);
    }
        
    // expose the operation
    return lookup;
}

/**
 *  Calculate the delay until the next job
 *  @return double      the delay in seconds (or < 0 if there is no need to run a timer)
 */
double Core::delay(double now)
{
    // if there is an unprocessed inbound queue, we have to expire asap
    if (_ipv4.active() || _ipv6.active()) return 0.0;
    
    // if there is nothing scheduled
    if (_lookups.empty() && _ready.empty()) return -1.0;
    
    // if only one is set
    if (_lookups.empty()) return _ready.front()->delay(now);
    if (_ready.empty()) return _lookups.front()->delay(now);
    
    // get the minimum
    return std::min(_lookups.front()->delay(now), _ready.front()->delay(now));
}

/**
 *  Set the timer to a certain period
 *  @param  seconds     expire time
 */
void Core::timer(double seconds)
{
    // if timer was not set and will not be set
    if (seconds < 0.0 && _timer == nullptr) return;
    
    // if timer was immediate and stays immediate, not changes are needed
    if (seconds == 0.0 && _timer != nullptr && _immediate) return;

    // if the timer is already running we have to reset it
    if (_timer != nullptr) _loop->cancel(_timer, this);
    
    // check when the next operation should run
    _timer = seconds < 0 ? nullptr : _loop->timer(seconds, this);
    _immediate = seconds == 0.0;
}

/**
 *  Reschedule the timer
 *  @param  now         current time
 */
void Core::reschedule(double now)
{
    // calculate the delay
    timer(delay(now));
}

/**
 *  Method that is called when a UDP socket has a buffer that it wants to deliver
 *  @param  sockets     the sockets with a buffer
 */
void Core::onActive(Sockets *sockets)
{
    // if we already had an immediate timer we do not have to set it
    if (_timer != nullptr && _immediate) return;

    // if the timer is already running we have to reset it
    if (_timer != nullptr) _loop->cancel(_timer, this);

    // check when the next operation should run
    _timer = _loop->timer(0.0, this);
    _immediate = true;
}

/**
 *  Process a lookup, which means that the next action for the lookup should be taken (like repeating a datagram or timing out)
 *  @param  watcher     object to monitor if `this` was destructed
 *  @param  lookup      the lookup to process
 *  @param  now         current time
 *  @return bool        was this lookup indeed processable (false if processed too early)
 */
bool Core::process(const Watcher &watcher, const std::shared_ptr<Lookup> &lookup, double now)
{
    // if this lookup was already finished (this just pops it off the queue), this happens because we only
    // pop messages from the queues, and lookups in the middle might already be finished by the time the
    // hit the front of the queue)
    if (lookup->finished()) return true;
    
    // if it is not yet time to run this lookup, we do nothing more
    if (lookup->delay(now) > 0.0) return false;

    // run the lookup (if this succeeds a call to userspace was made, which means the operation is done)
    bool success = lookup->execute(now);
    
    // if user-space destructed `this` there is nothing else to do
    if (success && !watcher.valid()) return true;
    
    // update counter if call was completed
    if (success) _inflight -= 1;

    // if no more attempts are expected, we put it in a special list
    else if (lookup->exhausted()) _ready.push_back(lookup);
    
    // remember the lookup for the next attempt
    else _lookups.push_back(lookup);
    
    // done
    return true;
}

/**
 *  Proceed with more operations
 *  @param  watcher
 *  @param  now
 */
void Core::proceed(const Watcher &watcher, double now)
{
    // make sure this is absolutely true or we'll end up iterating for a long time
    assert(_inflight <= _capacity);

    // iterate
    while (watcher.valid() && _capacity > _inflight && !_scheduled.empty())
    {
        // the lookup that will be started
        auto lookup = _scheduled.front();

        // this lookup is no longer scheduled
        _scheduled.pop_front();
        
        // it is possible that scheduled operations are already cancelled
        if (lookup->finished()) continue;
        
        // this is now in progress
        _inflight += 1;
        
        // run it, the process() always returns true
        process(watcher, lookup, now);
    }
}

/**
 *  Method that is called when the timer expires
 */
void Core::expire()
{
    // forget the timer
    _loop->cancel(_timer, this); _timer = nullptr;
    
    // a call to userspace might destruct `this`
    Watcher watcher(this);
    
    // get the current time
    Now now;
    
    // first we check the udp sockets to see if they have data availeble
    size_t ipv4calls = _ipv4.deliver(_maxcalls); if (!watcher.valid()) return;
    size_t ipv6calls = _ipv6.deliver(_maxcalls - ipv4calls); if (!watcher.valid()) return;
    
    // if there were calls to userspace, we must update our bookkeeping
    _inflight -= (ipv4calls + ipv6calls);

    // number of calls to userspace left
    size_t callsleft = _maxcalls - ipv4calls - ipv6calls;

    // there was no data to process, so we are going to run jobs
    while (callsleft > 0 && !_lookups.empty())
    {
        // get the oldest operation from the queue
        if (!process(watcher, _lookups.front(), now)) break;
        
        // maybe the userspace call ended up in `this` being destructed
        if (!watcher.valid()) return;
        
        // log one extra call 
        // @todo this is not entirely correct, maybe there was no call to userspace
        callsleft -= 1;

        // forget this lookup because we ran it
        // @todo what if there was no call to userspace?
        _lookups.pop_front();
    }
    
    // look at lookups that can no longer be repeated, but for which we're waiting for answer
    while (callsleft > 0 && !_ready.empty())
    {
        // get the oldest operation
        if (!process(watcher, _ready.front(), now)) break;

        // maybe the userspace call ended up in `this` being destructed
        if (!watcher.valid()) return;

        // log one extra call
        callsleft -= 1;
        
        // forget this lookup because we are going to run it
        _ready.pop_front();
    }

    // execute more lookups if possible
    proceed(watcher, now);

    // reset the timer
    reschedule(now);
}

/**
 *  Send a message over a UDP socket
 *  @param  ip              target IP
 *  @param  query           the query to send
 *  @return Inbound         the object that receives the answer
 */
Inbound *Core::datagram(const Ip &ip, const Query &query)
{
    // check the version number of ip
    switch (ip.version()) {
    case 4:     return _ipv4.datagram(ip, query);
    case 6:     return _ipv6.datagram(ip, query);
    default:    return nullptr;
    }
}

/**
 *  Connect with TCP to a socket
 *  This is an async operation, the connection will later be passed to the connector
 *  @param  ip          IP address of the target nameservers
 *  @param  connector   the object interested in the connection
 *  @return Connecting  the socket to which we're subscribing
 */
Connecting *Core::connect(const Ip &ip, Connector *connector)
{
    // check the version number of ip
    switch (ip.version()) {
    case 4:     return _ipv4.connect(ip, connector);
    case 6:     return _ipv6.connect(ip, connector);
    default:    return nullptr;
    }
}

/**
 *  Should the search path be respected?
 *  @param  config      configuration
 *  @param  domain      the domain to lookup
 *  @param  handler     handler that is already in use
 *  @return bool
 */
bool Core::searchable(const std::shared_ptr<Config> &config, const char *domain, DNS::Handler *handler)
{
    // length of the lookup
    size_t length = strlen(domain);
    
    // empty domains fail anyway
    if (length == 0) return false;
    
    // canonical domains don't go into recursion
    if (domain[length-1] == '.') return false;
    
    // count the dots
    size_t ndots = std::count(domain, domain + length + 1, '.');
    
    // compare with the 'ndots' setting
    if (ndots >= config->ndots()) return false;
    
    // do not do recursion (if the current handler already is a SearchLookup)
    return dynamic_cast<SearchLookup*>(handler) == nullptr;
}

/**
 *  Do a dns lookup
 *  @param  domain      the record name to look for
 *  @param  type        type of record (normally you ask for an 'a' record)
 *  @param  bits        bits to include in the query
 *  @param  handler     object that will be notified when the query is ready
 *  @return Operation   object to interact with the operation while it is in progress
 */
Operation *Core::query(const std::shared_ptr<Config> &config, const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler)
{
    // check if we should respect the search path
    if (searchable(config, domain, handler)) return new SearchLookup(this, config, type, bits, domain, handler);
    
    // for A and AAAA lookups we also check the /etc/hosts file
    if (type == ns_t_a    && config->hosts().lookup(domain, 4)) return add(new LocalLookup(this, config, domain, type, handler));
    if (type == ns_t_aaaa && config->hosts().lookup(domain, 6)) return add(new LocalLookup(this, config, domain, type, handler));
    
    // the request can throw (for example when the domain is invalid
    try
    {
        // we are going to create a self-destructing request
        return add(new RemoteLookup(this, config, domain, type, bits, handler));
    }
    catch (...)
    {
        // invalid parameters were supplied
        return nullptr;
    }
}

/**
 *  Do a reverse IP lookup, this is only meaningful for PTR lookups
 *  @param  config      configuration to use
 *  @param  ip          the ip address to lookup
 *  @param  bits        bits to include in the query
 *  @param  handler     object that will be notified when the query is ready
 *  @return operation   object to interact with the operation while it is in progress
 */
Operation *Core::query(const std::shared_ptr<Config> &config, const Ip &ip, const Bits &bits, DNS::Handler *handler)
{
    // if the /etc/hosts file already holds a record
    if (config->hosts().lookup(ip)) return add(new LocalLookup(this, config, ip, handler));

    // pass on to the regular query method
    return query(config, Reverse(ip), TYPE_PTR, bits, handler);
}

/**
 *  Mark a lookup as cancelled and start more queues lookups
 *  @param  lookup
 */
void Core::cancel(const Lookup *lookup)
{
    // if the operation was not yet included in the _inflight counter
    if (lookup->scheduled()) return;
    
    // one operation less active
    _inflight -= 1;
    
    // is there room for more operations, and do we have them?
    if (_inflight >= _capacity || _scheduled.empty()) return;
    
    // start a timer to start more operations
    timer(0.0);
}

/**
 *  End of namespace
 */
}

