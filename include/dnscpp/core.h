/**
 *  Core.h
 * 
 *  The core is the private base class of the dns context and is used
 *  internally by the dns-cpp library. This class is not accessible
 *  in user space. The public methods in this class can therefore not
 *  be called from user space, but they are used internally.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2025 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "sockets.h"
#include "resolvconf.h"
#include "hosts.h"
#include "bits.h"
#include "now.h"
#include "lookup.h"
#include "processor.h"
#include "timer.h"
#include "config.h"
#include <list>
#include <deque>
#include <memory>
#include <cassert>

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Forward declarations
 */
class Loop;

/**
 *  Class definition
 */
class Core : private Timer, private Watchable, private Sockets::Handler
{
protected:
    /**
     *  Pointer to the event loop supplied by user space
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  Collection of sockets (we need two for ipv4 and ipv6 traffic)
     *  @var Sockets
     * 
     *  @todo this is stupid
     */
    Sockets _ipv4;
    Sockets _ipv6;

    /**
     *  The configuration
     *  @var std::shared_ptr<Config>
     */
    std::shared_ptr<Config> _config;

    /**
     *  All operations that are in progress and that are waiting for the next 
     *  (possibly first) attempt. Note that we use multiple queues so that we do
     *  not have to use a slow (priority) queue.
     *  @var std::deque<std::shared_ptr<Lookup>>
     */
    std::deque<std::shared_ptr<Lookup>> _lookups;
    
    /**
     *  To avoid that external DNS servers, or our own response-buffer, is flooded
     *  with data, there is a limit on the number of operations that can run. If
     *  there are more operations than we can handle, this buffer is used for 
     *  overflow (is not supposed to happen often!)
     *  @var std::deque<std::shared_ptr<Lookup>>
     */
    std::deque<std::shared_ptr<Lookup>> _scheduled;
    
    /**
     *  Lookups for which the max number of attempts have been reached (no further
     *  messages will be sent) and that are waiting for response or expiration
     *  @var std::deque<std::shared_ptr<Lookup>>
     */
    std::deque<std::shared_ptr<Lookup>> _ready;
    
    /**
     *  The next timer to run
     *  @var void *
     */
    void *_timer = nullptr;
    
    /**
     *  Is the timer expected to expire right away
     *  @var bool
     */
    bool _immediate = false;

    /**
     *  Max number of operations to run at the same time
     *  @var size_t
     */
    size_t _capacity = 1024;

    /**
     *  The max number of calls to be made to userspace in one iteration
     *  @var size_t
     */
    size_t _maxcalls = 64;

    /**
     *  Number of lookups inflight
     *  @var size_t
     */
    size_t _inflight = 0;

    /**
     *  Calculate the delay until the next job
     *  @return double      the delay in seconds (or < 0 if there is no need to run a timer)
     */
    double delay(double now);

    /**
     *  Set the timer to a certain period
     *  @param  seconds     expire time
     */
    void timer(double seconds);

    /**
     *  Proceed with more operations
     *  @param  watcher
     *  @param  now
     */
    void proceed(const Watcher &watcher, double now);
    
    /**
     *  Process a lookup
     *  @param  watcher     object to monitor if `this` was destructed
     *  @param  lookup      the lookup to process
     *  @param  now         current time
     *  @return bool        was this lookup indeed processable (false if processed too early)
     */
    bool process(const Watcher &watcher, const std::shared_ptr<Lookup> &lookup, double now);

    /**
     *  Notify the timer that it expired
     */
    virtual void expire() override;
    
    /**
     *  Add a new lookup to the list
     *  @param  lookup
     *  @return Operation
     */
    Operation *add(Lookup *lookup);
    

public:
    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     *  @param  config      the configuration
     * 
     *  @deprecated
     */
    Core(Loop *loop, const std::shared_ptr<Config> &config);

    /**
     *  No copying
     *  @param  that
     */
    Core(const Core &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Core();

    /**
     *  Method that is called when a UDP socket has a buffer that it wants to deliver,
     *  or is otherwise in need of action (like in a lost state)
     *  @param  sockets     the sockets with a buffer
     */
    void onActive(Sockets *sockets) override;

    /**
     *  Method that is called when a UDP socket has a buffer that it wants to deliver
     *  @param  now         current time
     */
    void reschedule(double now);

    /**
     *  Expose the event loop
     *  @return Loop
     */
    Loop *loop() { return _loop; }
    
    /**
     *  The period between sending the datagram again
     *  @return double
     */
    double interval() const { return _config->interval(); }
    
    /**
     *  The time to wait for a response
     *  @return double
     */
    double timeout() const { return _config->timeout(); }
    
    /**
     *  Max number of attempts / number of requests to send
     *  @return size_t
     */
    size_t attempts() const { return _config->attempts(); }
    
    /**
     *  THe capacity: number of operations to run at the same time
     *  @return size_t
     */
    size_t capacity() const { return _capacity; }
    
    /**
     *  Should all nameservers be rotated? otherwise they will be tried in-order
     *  @var bool
     */
    bool rotate() const { return _config->rotate(); }

    /**
     *  Does a certain hostname exists in /etc/hosts? In that case a NXDOMAIN error should not be given
     *  @param  hostname        hostname to check
     *  @return bool            does it exists in /etc/hosts?
     */
    bool exists(const char *hostname) const { return _config->exists(hostname); }

    /**
     *  Send a message over a UDP socket
     *  @param  ip              target IP
     *  @param  query           the query to send
     *  @return Inbound         the object that receives the answer
     */
    Inbound *datagram(const Ip &ip, const Query &query);

    /**
     *  Connect with TCP to a socket
     *  This is an async operation, the connection will later be passed to the connector
     *  @param  ip              IP address of the target nameservers
     *  @param  connector       the object interested in the connection
     *  @return Connecting*     object to which the caller is subscribed
     */
    Connecting *connect(const Ip &ip, Connector *connector);

    /**
     *  Mark a lookup as cancelled and start more queues lookups
     *  This is called internally when userspace cancels a single operation (via Operation::cancel())
     *  @param  lookup
     */
    void cancel(const Lookup *lookup);
};

/**
 *  End of namespace
 */
}
