/**
 *  Watchable.h
 *
 *  Every class that overrides from the Watchable class can be monitored for
 *  destruction by a Monitor object
 *
 *  @copyright 2014 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <vector>
#include <algorithm>

/**
 *  Set up namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Watcher;

/**
 *  Class definition
 */
class Watchable
{
private:
    /**
     *  The monitors
     *  @var std::vector
     */
    std::vector<Watcher*> _monitors;

    /**
     *  Add a monitor
     *  @param  monitor
     */
    void add(Watcher *monitor)
    {
        // add to the vector
        _monitors.push_back(monitor);
    }

    /**
     *  Remove a monitor
     *  @param  monitor
     */
    void remove(Watcher *monitor)
    {
        // put the monitor at the end of the vector
        auto iter = std::remove(_monitors.begin(), _monitors.end(), monitor);

        // make the vector smaller
        _monitors.erase(iter, _monitors.end());
    }

public:
    /**
     *  Destructor
     */
    virtual ~Watchable();

    /**
     *  Only a monitor has full access
     */
    friend class Watcher;
};

/**
 *  End of namespace
 */
}
