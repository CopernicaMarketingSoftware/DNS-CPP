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

    /**
     *  Should the search path be respected?
     *  @param  domain      the domain to lookup
     *  @param  handler     handler that is already in use
     *  @return bool
     */
    static bool searchable(const std::shared_ptr<Config> &config, const char *domain, DNS::Handler *handler);
    

public:
    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     */
    Core(Loop *loop);

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
     *  Number of sockets to use
     *  This is normally 1 which is enough for most applications. However,
     *  for applications that send many UDP requests (new requests are sent
     *  before the previous ones are completed, this number could be set higher
     *  to ensure that the load is spread out over multiple sockets that are 
     *  closed and opened every now and then to ensure that port numbers are
     *  refreshed. You can only use this to _increment_ the number of sockets.
     *  @param  count       number of sockets
     */
    void sockets(size_t count)
    {
        // pass on
        _ipv4.sockets(count);
        _ipv6.sockets(count);
    }

    /**
     *  Set the max number of calls that are made to userspace in one iteration
     *  @param  value       the new value
     */
    void maxcalls(size_t value) { _maxcalls = value; }
    
    /**
     *  THe capacity: number of operations to run at the same time
     *  @return size_t
     */
    size_t capacity() const { return _capacity; }

    /**
     *  Set the capacity: number of operations to run at the same time
     *  @param  value       the new value
     */
    void capacity(size_t value);

    /**
     *  Set the send & receive buffer size of each individual UDP socket
     *  @param value  the value to set
     */
    void buffersize(int32_t value)
    {
        // pass to the actual sockets
        _ipv4.buffersize(value);
        _ipv6.buffersize(value);
    }
    
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
     *  Do a dns lookup
     *  @param  config      configuration to use
     *  @param  domain      the record name to look for
     *  @param  type        type of record (normally you ask for an 'a' record)
     *  @param  bits        bits to include in the query
     *  @param  handler     object that will be notified when the query is ready
     *  @return Operation   object to interact with the operation while it is in progress
     */
    Operation *query(const std::shared_ptr<Config> &config, const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler);

    /**
     *  Do a reverse IP lookup, this is only meaningful for PTR lookups
     *  @param  config      configuration to use
     *  @param  ip          the ip address to lookup
     *  @param  bits        bits to include in the query
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const std::shared_ptr<Config> &config, const Ip &ip, const Bits &bits, DNS::Handler *handler);

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
