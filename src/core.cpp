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
 *  @throws std::runtime_error
 */
Core::Core(Loop *loop, bool defaults) : _loop(loop), _udp(loop, this)
{
    // do nothing if we don't need the defaults
    if (!defaults) return;
    
    // load the defaults from /etc/resolv.conf
    ResolvConf settings;
    
    // copy the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(this, settings.nameserver(i), &_udp);
    
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
 */
Core::Core(Loop *loop, const ResolvConf &settings) : _loop(loop), _udp(loop, this)
{
    // construct the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(this, settings.nameserver(i), &_udp);

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
 *  Method that is called when a response is received
 *  @param  time        receive-time
 *  @param  address     the address of the nameserver from which it is received
 *  @param  response    the received response
 *  @param  size        size of the response
 */
void Core::onReceived(time_t now, const struct sockaddr *addr, const unsigned char *response, size_t size)
{
    // parse the IP
    const Ip ip(addr);

    // find the nameserver that sent this response
    for (auto &nameserver : _nameservers)
    {
        // continue to the next nameserver if the response was not from this nameserver
        if (nameserver.ip() != ip) continue;

        // we found the nameserver that sent this response
        nameserver.receive(response, size);

        // we need to process this queue
        reschedule(now);

        // we can jump out of the loop now
        break;
    }
}

/**
 *  Add a new lookup to the list
 *  @param  lookup
 *  @return Operation
 */
Operation *Core::add(Lookup *lookup)
{
    // add to the operations
    if (_lookups.size() < _capacity)
    {
        // put at the beginning of the list (because we want to run it immediately)
        _lookups.emplace_front(lookup);
        
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
    // check if there are any nameservers with 
    for (auto &nameserver : _nameservers)
    {
        // is there an unprocessed queue, than we have to expire asap
        if (nameserver.busy()) return 0.0;
    }
    
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
 *  Process a lookup
 *  @param  lookup      the lookup to process
 *  @param  now         current time
 *  @return bool        was this lookup indeed processable (false if processed too early)
 */
bool Core::process(const std::shared_ptr<Lookup> &lookup, double now)
{
    // if it is not yet time to run this lookup, we do nothing more
    if (lookup->delay(now) > 0.0) return false;

    // run the lookup (if this fails the lookup was already finished and we do not have to reschedule it)
    if (!lookup->execute(now)) return true;
    
    // if no more attempts are expected, we put it in a special list
    if (lookup->credits() == 0) _ready.push_back(lookup);
    
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
void Core::proceed(double now, size_t count)
{
    // iterate
    while (count > 0)
    {
        // not possible if nothing is scheduled
        if (_scheduled.empty()) return;
        
        // get the oldest scheduled operation (the process() always returns true)
        if (!process(_scheduled.front(), now)) return;
        
        // this lookup is no longer scheduled
        _scheduled.pop_front();
        
        // one extra operation is scheduled
        count -= 1;
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
    
    // number of calls made
    size_t calls = 0;
    
    // first we are going to check the nameservers if they have some data to process
    for (auto &nameserver : _nameservers)
    {
        // because processing a response may lead to user-space destructing everything,
        // we leap out if there was indeed something processed
        size_t count = nameserver.process(_maxcalls - calls);
        
        // if nothing was processed we move one
        if (count == 0) continue;
        
        // something was processed, is the side-effect that userspace destucted `this`?
        if (!watcher.valid()) return;

        // update bookkeeping (this is not entirely correct, maybe there was no call to userspace)
        calls += count;

        // start other operations now that some earlier operations are completed
        proceed(now, count);
        
        // is it meaningful to proceed
        if (calls > _maxcalls) break;        
    }
    
    // there was no data to process, so we are going to run jobs
    while (calls < _maxcalls && !_lookups.empty())
    {
        // get the oldest operation
        if (!process(_lookups.front(), now)) break;
        
        // maybe the userspace call ended up in `this` being destructed
        if (!watcher.valid()) return;
        
        // log one extra call (this is not entirely correct, maybe there was no call to userspace)
        calls += 1;
        
        // forget this lookup because we ran it
        _lookups.pop_front();
    }
    
    // look at lookups that can no longer be repeated, but for which we're waiting for answer
    while (calls < _maxcalls && !_ready.empty())
    {
        // get the oldest operation
        if (!process(_ready.front(), now)) break;

        // maybe the userspace call ended up in `this` being destructed
        if (!watcher.valid()) return;

        // log one extra call
        calls += 1;
        
        // forget this lookup because we are going to run it
        _ready.pop_front();
    }

    // if nothing is inflight we can close the socket
    if (_lookups.empty() && _ready.empty()) _udp.close();

    // if there are more slots for scheduled operations, we start them now
    if (_capacity > _lookups.size()) proceed(now, _capacity - _lookups.size());
    
    // reset the timer
    reschedule(now);
}

/**
 *  End of namespace
 */
}

