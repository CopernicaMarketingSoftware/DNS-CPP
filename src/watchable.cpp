/**
 *  Watchable.cpp
 *
 *  @copyright 2014 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/watchable.h"
#include "../include/dnscpp/watcher.h"

/**
 *  Set up namespace
 */
namespace DNS {

/**
 *  Destructor
 */
Watchable::~Watchable()
{
    // loop through all monitors
    for (auto iter = _monitors.begin(); iter != _monitors.end(); iter++)
    {
        // tell the monitor that it now is invalid
        (*iter)->invalidate();
    }
}

/**
 *  End of namespace
 */
}

