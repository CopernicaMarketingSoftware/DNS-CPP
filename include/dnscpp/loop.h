/**
 *  Loop.h
 * 
 *  The DNS-CPP library is eventloop-agnostic. It is up to the caller 
 *  to proved an event loop that satisfies the interface as defined
 *  by the class defined in this file.
 * 
 *  If you're using an existing event loop like libevent, libuv, etc,
 *  you need to wrap it in a class that extends from this class.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Forward declarations
 */
class Monitor;
class Timer;

/**
 *  Class definition
 */
class Loop
{
public:
    /**
     *  Constructor
     */
    Loop() = default;
    
    /**
     *  Destructor
     */
    virtual ~Loop() = default;
    
    /**
     *  Add a filedescriptor to the event loop
     * 
     *  This method is called when the dns library wants a filedescriptor to 
     *  be monitored for activity. When the filedescriptor becomes readable or
     *  writable, you should call the Monitor::notify() method.
     * 
     *  You should return a void-pointer to identify the filedescriptor. This
     *  pointer will then be passed to subsequent calls to the update() method if
     *  the events have to be changed for that filedescriptor.
     * 
     *  @param  int     file-descriptor to be monitored
     *  @param  int     the events to monitor (1 = readability, 2 = writability) 
     *  @param  Monitor object that should be notified when the filedescriptor becomes active
     *  @return void*   identifier of the watcher in the event loop
     */
    virtual void *add(int fd, int events, Monitor *monitor) = 0;
    
    /**
     *  Update the events for which a filedescriptor are monitored
     *  
     *  This method is called after the add() method, when the DNS library
     *  wants to monitor other events for a filedescriptor
     * 
     *  @param  void*   the identifier returned by the previous call to add()
     *  @param  int     file-descriptor for which the events are changed
     *  @param  int     the new events to monitor (1 = readabilty, 2 = writability)
     *  @param  Monitor object that should be notified when the filedescriptor becomes active
     *  @return void*   identified of the watcher in the event loop
     */
    virtual void *update(void *identifier, int fd, int events, Monitor *monitor) = 0;
    
    /**
     *  Remove a filedescriptor from the event loop
     * 
     *  @param  void*   the identifier returned by the previous call to add()
     *  @param  int     the file-descriptor to be removed
     *  @param  Monitor the object that is no longer interested
     */
    virtual void remove(void *identifier, int fd, Monitor *monitor) = 0;
    
    /**
     *  Set a timer
     *  @param  timeout number of seconds after which the timer expires
     *  @param  Timer the object that should be notified when the timer expired
     *  @return void*   identifier for the timer
     */
    virtual void *timer(double timeout, Timer *timer) = 0;
    
    /**
     *  Method that is called when a timer is cancelled. This is called when
     *  the DNS library no longer needs to be notified.
     * 
     *  @param  void*   identifier of the timer (returned by the timer() method)
     *  @param  Timer   the timer to cancel
     */
    virtual void cancel(void *identifier, Timer *timer) = 0;
};
    
/**
 *  End of namespace
 */
}

