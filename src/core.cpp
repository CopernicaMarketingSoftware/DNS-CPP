/**
 *  Core.cpp
 * 
 *  Implementation file for the Core class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/lookup.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/watcher.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        your event loop
 *  @param  defaults    should defaults from resolv.conf and /etc/hosts be loaded?
 *  @param  buffersize  send & receive buffer size of each UDP socket
 *  @throws std::runtime_error
 */
Core::Core(Loop *loop, bool defaults) :
    _loop(loop),
    _ipv4(loop, this),
    _ipv6(loop, this)
{
    // do nothing if we don't need the defaults
    if (!defaults) return;
    
    // load the defaults from /etc/resolv.conf
    ResolvConf settings;
    
    // copy the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(settings.nameserver(i));
    
    // take over some of the settings
    _timeout = settings.timeout();
    _interval = settings.timeout();
    _attempts = settings.attempts();
    _rotate = settings.rotate();

    // we also have to load /etc/hosts
    if (!_hosts.load()) throw std::runtime_error("failed to load /etc/hosts");
}

/**
 *  Protected constructor, only the derived class may construct it
 *  @param  loop        your event loop
 *  @param  settings    settings from the resolv.conf file
 *  @param  buffersize  send & receive buffer size of each UDP socket
 */
Core::Core(Loop *loop, const ResolvConf &settings) :
    _loop(loop),
    _ipv4(loop, this),
    _ipv6(loop, this)
{
    // construct the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(settings.nameserver(i));

    // take over some of the settings
    _timeout = settings.timeout();
    _interval = settings.timeout();
    _attempts = settings.attempts();
    _rotate = settings.rotate();
}


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
 *  Add a new lookup to the list
 *  @param  lookup
 *  @return Operation
 */
Operation *Core::add(Lookup *lookup)
{
    // add to the operations
    if (_inflight < _capacity)
    {
        // put at the beginning of the list (because we want to run it immediately)
        _lookups.emplace_front(lookup);
        
        // we consider this lookup to be active (although we still have to make the first execute() call)
        _inflight += 1;

        // if we already have a timer the expires immediately
        if (_timer && _immediate) return lookup;
    
        // stop existing timer
        if (_timer) _loop->cancel(_timer, this);
        
        // reschedule the timer
        _timer = _loop->timer(0.0, this);
        
        // this is an immediate-timer
        _immediate = true;
    }
    else
    {
        // we already have too many operations in progress, delay it
        _scheduled.emplace_back(lookup);
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
    if (_ipv4.buffered() || _ipv6.buffered()) return 0.0;
    
    // if there is nothing scheduled
    if (_lookups.empty() && _ready.empty()) return -1.0;
    
    // if only one is set
    if (_lookups.empty()) return _ready.front()->delay(now);
    if (_ready.empty()) return _lookups.front()->delay(now);
    
    // get the minimum
    return std::min(_lookups.front()->delay(now), _ready.front()->delay(now));
}

/**
 *  Reschedule the timer
 *  @param  now         current time
 */
void Core::reschedule(double now)
{
    // calculate the delay
    auto seconds = delay(now);
    
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
 *  Method that is called when a UDP socket has a buffer that it wants to deliver
 *  @param  udp         the socket with a buffer
 */
void Core::onBuffered(Udps *udp)
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
 *  @param  lookup      the lookup to process
 *  @param  now         current time
 *  @return bool        was this lookup indeed processable (false if processed too early)
 */
bool Core::process(const std::shared_ptr<Lookup> &lookup, double now)
{
    // if it is not yet time to run this lookup, we do nothing more
    if (lookup->delay(now) > 0.0) return false;

    // run the lookup (if this succeeds a call to userspace was made, which means the operation is done)
    // @todo watch out this could destruct 'this'
    if (lookup->execute(now)) _inflight -= 1;

    // if no more attempts are expected, we put it in a special list
    else if (lookup->exhausted()) _ready.push_back(lookup);
    
    // remember the lookup for the next attempt
    else _lookups.push_back(lookup);
    
    // done
    return true;
}

/**
 *  Proceed with more operations
 *  @param  now
 *  @param  count
 */
void Core::proceed(double now)
{
    // make sure this is absolutely true or we'll end up iterating for a long time
    assert(_inflight <= _capacity);

    // iterate
    while (_capacity > _inflight && !_scheduled.empty())
    {
        // the lookup that will be started
        auto lookup = _scheduled.front();
        
        // this is now in progress
        _inflight += 1;

        // this lookup is no longer scheduled
        _scheduled.pop_front();
        
        // get the oldest scheduled operation (the process() always returns true @todo really?)
        // @todo could this result in a destruct of `this`?
        if (!process(_scheduled.front(), now)) return;
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
        if (!process(_lookups.front(), now)) break;

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
        if (!process(_ready.front(), now)) break;

        // maybe the userspace call ended up in `this` being destructed
        if (!watcher.valid()) return;

        // log one extra call
        callsleft -= 1;
        
        // forget this lookup because we are going to run it
        _ready.pop_front();
    }

    // execute more lookups if possible
    proceed(now);

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
    case 4:     return _ipv4.send(ip, query);
    case 6:     return _ipv6.send(ip, query);
    default:    return nullptr;
    }
}

/**
 *  End of namespace
 */
}

