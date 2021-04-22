/**
 *  LibEv.h
 *
 *  Example eventloop implementation for a libev based event loop
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Incude guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "monitor.h"
#include "timer.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class LibEv : public Loop
{
private:
    /**
     *  The actual libev event loop
     *  @var ev_loop
     */
    struct ev_loop *_loop;
    
    /**
     *  Should the operations persist the loop?
     *  @var bool
     */
    bool _persist;

    /**
     *  Callback method that is called when a filedescriptor becomes active
     *  @param  loop        The loop in which the event was triggered
     *  @param  w           Internal watcher object
     *  @param  revents     Events triggered
     */
    static void active(struct ev_loop *loop, ev_io *watcher, int revents)
    {
        // retrieve the monitor
        Monitor *monitor = (Monitor *)watcher->data;

        // notify the monitor
        monitor->notify();
    }

    /**
     *  Callback method that is called when the timer expires
     *  @param  loop        The loop in which the event was triggered
     *  @param  w           Internal watcher object
     *  @param  revents     Events triggered
     */
    static void expire(struct ev_loop *loop, ev_timer *watcher, int revents)
    {
        // retrieve the timer
        Timer *timer = (Timer *)watcher->data;

        // notify the timer
        timer->expire();
    }

public:
    /**
     *  Constructor
     *  You can pass a boolean value to make the event loop persistent. This means
     *  that when an operation is in progress, the event loop automatically stays
     *  active. If persist=false the watchers that are created on the event loop
     *  do not keep the event loop alive.
     *  @param  loop        the libev event loop that is wrapped
     *  @param  persist     make the event loop persistent
     */
    LibEv(struct ev_loop *loop, bool persist = true) : _loop(loop), _persist(persist) {}
    
    /**
     *  No copying
     *  @param  that
     */
    LibEv(const LibEv &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~LibEv() = default;
    
    /**
     *  Add a filedescriptor to the event loop
     *  @param  int     file-descriptor to be monitored
     *  @param  int     the events to monitor (1 = readability, 2 = writability) 
     *  @param  Monitor object that should be notified when the filedescriptor becomes active
     *  @return void*   identifier of the watcher in the event loop
     */
    virtual void *add(int fd, int events, Monitor *monitor) override
    {
        // construct the watcher object
        ev_io *watcher = (ev_io *)malloc(sizeof(ev_io));
        
        // associate the monitor with the watcher
        watcher->data = monitor;
        
        // initialize the watcher
        ev_io_init(watcher, active, fd, events);
        
        // start monitoring for activity
        ev_io_start(_loop, watcher);
        
        // dont affect refcount
        if (!_persist) ev_unref(_loop);
        
        // expose the watcher as identifier
        return watcher;
    }
    
    /**
     *  Update the events for which a filedescriptor are monitored
     *  @param  void*   the identifier returned by the previous call to add()
     *  @param  int     file-descriptor for which the events are changed
     *  @param  int     the new events to monitor (1 = readabilty, 2 = writability)
     *  @param  Monitor object that should be notified when the filedescriptor becomes active
     *  @return void*   identified of the watcher in the event loop
     */
    virtual void *update(void *identifier, int fd, int events, Monitor *monitor) override
    {
        // the identifier is a watcher
        ev_io *watcher = (ev_io *)identifier;
        
        // stop the watcher first
        ev_io_stop(_loop, watcher);
        
        // we update the monitor too (in reality the monitor object never changes)
        watcher->data = monitor;
        
        // change the events 
        ev_io_set(watcher, fd, events);
        
        // restart it
        ev_io_start(_loop, watcher);
        
        // expose the same identifier
        return identifier;
    }
    
    /**
     *  Remove a filedescriptor from the event loop
     *
     *  @param  void*   the identifier returned by the previous call to add()
     *  @param  int     the file-descriptor to be removed
     *  @param  Monitor the object that is no longer interested
     */
    virtual void remove(void *identifier, int fd, Monitor *monitor) override
    {
        // restore refcount
        if (!_persist) ev_ref(_loop);
        
        // the identifier is a watcher
        ev_io *watcher = (ev_io *)identifier;
        
        // remove the watcher from the event loop
        ev_io_stop(_loop, watcher);
        
        // we no longer need the watcher
        free(watcher);
    }
    
    /**
     *  Set a timer
     *  @param  timeout number of seconds after which the timer expires
     *  @param  Timer   the object that should be notified when the timer expired
     *  @return void*   identifier for the timer
     */
    virtual void *timer(double timeout, Timer *timer) override
    {
        // construct the watcher object
        ev_timer *watcher = (ev_timer *)malloc(sizeof(ev_timer));
        
        // associate the timer with the watcher
        watcher->data = timer;
        
        // initialize the watcher
        ev_timer_init(watcher, expire, timeout, 0.0);
        
        // timers have a low priority (we rather wait a little longer if we're dealing with other data too)
        ev_set_priority(watcher, EV_MINPRI);
        
        // start monitoring for activity
        ev_timer_start(_loop, watcher);

        // dont affect refcount
        if (!_persist) ev_unref(_loop);
        
        // expose the watcher as identifier
        return watcher;
    }
    
    /**
     *  Method that is called when a timer is cancelled. This is called when
     *  the DNS library no longer needs to be notified.
     * 
     *  @param  void*   identifier of the timer (returned by the timer() method)
     *  @param  Timer   the object that is unregistered
     */
    virtual void cancel(void *identifier, Timer *timer) override
    {
        // restore refcount
        if (!_persist) ev_ref(_loop);

        // the identifier is a watcher
        ev_timer *watcher = (ev_timer *)identifier;
        
        // remove the watcher from the event loop
        ev_timer_stop(_loop, watcher);
        
        // we no longer need the watcher
        free(watcher);
    }
};
    
/**
 *  End of namespace
 */
}

