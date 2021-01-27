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
    
    // the timeout value is the time before moving on to the next server
    _spread = settings.timeout();

    // the attempts divided by the expire time will be the interval
    _interval = _expire / settings.attempts();

    // set the rotate setting
    _rotate = settings.rotate();

    // we also have to load /etc/hosts
    if (!_hosts.load()) throw std::runtime_error("failed to load /etc/hosts");
}

/**
 *  DNS-CPP keeps internal buffers with received, but not-yet processed messages
 *  This method returns the time when the oldest message in this buffer was received
 *  @return time_t
 */
time_t Core::oldest() const
{
    // result variable
    time_t result = 0;
    
    // check all nameservers
    for (const auto &nameserver : _nameservers)
    {
        // get the oldest buffered message from this ns
        auto oldest = nameserver.oldest();

        // do nothing if there is no buffer
        if (oldest == 0) continue;
        
        // do we have a better time?
        result = result == 0 ? oldest : std::min(result, oldest);
    }
    
    // done
    return result;
}

/**
 *  Time when the newest messag in the buffer was received
 *  @return time_t
 */
time_t Core::newest() const
{
    // result variable
    time_t result = 0;
    
    // check all nameservers
    for (const auto &nameserver : _nameservers)
    {
        // get the newest buffered message from this ns
        auto newest = nameserver.newest();

        // do nothing if there is no buffer
        if (newest == 0) continue;
        
        // do we have a better time?
        result = result == 0 ? newest : std::max(result, newest);
    }
    
    // done
    return result;
}
    
/**
 *  End of namespace
 */
}

