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
Core::Core(Loop *loop, bool defaults) : _loop(loop) 
{
    // do nothing if we don't need the defaults
    if (!defaults) return;
    
    // load the defaults from /etc/resolv.conf
    ResolvConf settings;
    
    // copy the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(this, settings.nameserver(i));
    
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
Core::Core(Loop *loop, const ResolvConf &settings) : _loop(loop) 
{
    // construct the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(this, settings.nameserver(i));

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
    if (_lookups.size() < _capacity)
    {
        // put at the beginning of the list (because we want to run it immediately)
        _lookups.emplace_front(lookup);
    
        // stop existing timer
        // @todo what if timer is already correct?
        if (_timer) _loop->cancel(_timer, this);
        
        // reschedule the timer
        _timer = _loop->timer(0.0, this);
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
 *  DNS-CPP keeps internal buffers with received, but not-yet processed messages
 *  This method checks if the buffer is up to date until a certain time (if there are
 *  no older message in the buffer)
 *  @param  time_t
 *  @return bool
 *
 *  @todo can we remove this?
 */
bool Core::uptodate(time_t time) const
{
    // check all nameservers
    for (const auto &nameserver : _nameservers)
    {
        // get the oldest buffered message from this ns
        auto oldest = nameserver.oldest();
        
        // check if the message was received before the check-time
        if (oldest != 0 && oldest <= time) return false;
    }
    
    // all buffers are up-to-date
    return true;
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
    // if the timer is already running we have to reset it
    if (_timer != nullptr) _loop->cancel(_timer, this);
    
    // calculate the delay
    auto seconds = delay(now);
    
    // check when the next operation should run
    _timer = seconds < 0 ? nullptr : _loop->timer(seconds, this);
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
        
        // get the oldest scheduled operation
        auto lookup = _scheduled.front();
        
        // this lookup is no longer scheduled
        _scheduled.pop_front();
        
        // execute it
        if (!lookup->execute(now)) continue;

        // the lookup indicated that it wants to run once more, so that
        // @todo check which queue to add it to
        _lookups.push_back(lookup);
        
        // one extra operation is scheduled
        count -= 1;
    }
}

/**
 *  Method that is called when the timer expires
 * 
 *  @todo rename because we already had an 'expire' method?
 */
void Core::expire()
{
    // a call to userspace might destruct `this`
    Watcher watcher(this);
    
    // get the current time
    Now now;
    
    // first we are going to check the nameservers if they have some data to process
    for (auto &nameserver : _nameservers)
    {
        // because processing a response may lead to user-space destructing everything,
        // we leap out if there was indeed something processed
        size_t count = nameserver.process();
        
        // if nothing was processed we move one
        if (count == 0) continue;
        
        // something was processed, is the side-effect that userspace destucted `this`?
        if (!watcher.valid()) return;
        
        // start other operations now that some earlier operations are completed
        proceed(now, count);
    }
    
    // there was no data to process, so we are going to run jobs
    // @todo fix hardcoded numbers
    for (size_t i = 0; i < 1000 && !_lookups.empty(); ++i)
    {
        // get the oldest operation
        auto lookup = _lookups.front();
        
        // if it is not yet time to run this lookup, we do nothing more
        if (lookup->delay(now) > 0.0) break;
        
        // forget this lookup because we are going to run it
        _lookups.pop_front();
        
        // run the lookup (if this fails the lookup was already finished and we do not have to reschedule it)
        if (!lookup->execute(now)) continue;
        
        // if no more attempts are expected, we put it in a special list
        if (lookup->attempts() >= _attempts) _ready.push_back(lookup);
        
        // remember the lookup for the next attempt
        else _lookups.push_back(lookup);
    }
    
    // look at lookups that can no longer be repeated, but for which we're waiting for answer
    // @todo fix hardcoded numbers
    for (size_t i = 0; i < 1000 && !_ready.empty(); ++i)
    {
        // get the oldest operation
        auto lookup = _ready.front();
        
        // if it is not yet time to run this lookup, we do nothing more
        if (lookup->delay(now) > 0.0) break;
    
        // forget this lookup because we are going to run it
        _ready.pop_front();
        
        // run the lookup for the last time
        lookup->execute(now);
    }

    // if there are more slots for scheduled operations, we start them now
    if (_capacity > _lookups.size()) proceed(now, _capacity - _lookups.size());
    
    // reset the timer
    reschedule(now);
}


    
/**
 *  End of namespace
 */
}

