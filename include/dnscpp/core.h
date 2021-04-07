/**
 *  Core.h
 * 
 *  The core is the private base class of the dns context and is used
 *  internally by the dns-cpp library. This class is not accessible
 *  in user space. The public methods in this class can therefore not
 *  be called from user space, but they are used internally.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "nameserver.h"
#include "resolvconf.h"
#include "hosts.h"
#include "bits.h"
#include "now.h"
#include "lookup.h"
#include <list>
#include <deque>
#include <memory>
#include <queue>
#include <random>
#include <set>

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
class Core : private Timer, private Watchable
{
protected:
    /**
     *  Pointer to the event loop supplied by user space
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  Source of randomness
     *  @var std::random_device
     */
    std::random_device _randomSource;

    /**
     *  Discrete uniform distribution for the range {1, 2, ..., 2^16 - 1}
     *  @var std::uniform_int_distribution<uint16_t>
     */
    std::uniform_int_distribution<uint16_t> _distribution;

    /**
     *  The IP addresses of the servers that can be accessed
     *  @var std::list<Nameserver>
     */
    std::list<Nameserver> _nameservers;
    
    /**
     *  The contents of the /etc/hosts file
     *  @var Hosts
     */
    Hosts _hosts;

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
     *  @var std::queue<std::shared_ptr<Lookup>>
     */
    std::queue<std::shared_ptr<Lookup>> _scheduled;
    
    /**
     *  Lookups for which the max number of attempts have been reached (no further
     *  messages will be sent) and that are waiting for response or expiration
     *  @var std::queue<std::shared_ptr<Lookup>>
     */
    std::queue<std::shared_ptr<Lookup>> _ready;

    /**
     *  The query IDs currently in flight.
     *  @var std::set<uint16_t>
     */
    std::set<uint16_t> _idsInUse;
    
    /**
     *  The next timer to run
     *  @var void *
     */
    void *_timer = nullptr;
    
    /**
     *  Is the timer expected to expire right away
     *  @var bool
     */
    double _immediate = false;
    
    /**
     *  Size of the send and receive buffer. If set to zero, default
     *  will be kept. This is limited by the system maximum (wmem_max and rmem_max)
     *  @var size_t
     */
    int32_t _buffersize = 0;

    /**
     *  Max time that we wait for a response
     *  @var double
     */
    double _timeout = 60.0;
    
    /**
     *  Max number of attempts / requests to send per query
     *  @var size_t
     */
    size_t _attempts = 5;

    /**
     *  Interval before a datagram is sent again
     *  @var double
     */
    double _interval = 2.0;
    
    /**
     *  Default bits to include in queries
     *  @var Bits
     */
    Bits _bits;
    
    /**
     *  Should all nameservers be rotated? otherwise they will be tried in-order
     *  @var bool
     */
    bool _rotate = false;
    
    /**
     *  Max number of operations to run at the same time
     *  @var size_t
     */
    size_t _capacity = 100;
    
    /**
     *  The max number of calls to be made to userspace in one iteration
     *  @var size_t
     */
    size_t _maxcalls = 5;
    

    /**
     *  Calculate the delay until the next job
     *  @return double      the delay in seconds (or < 0 if there is no need to run a timer)
     */
    double delay(double now);

    /**
     *  Proceed with more operations
     *  @param  now
     *  @param  count
     */
    void proceed(double now, size_t count);
    
    /**
     *  Process a lookup
     *  @param  lookup      the lookup to process
     *  @param  now         current time
     *  @return bool        was this lookup indeed processable (false if processed too early)
     */
    bool process(const std::shared_ptr<Lookup> &lookup, double now);

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
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     *  @param  defaults    should defaults from resolv.conf and /etc/hosts be loaded?
     *  @throws std::runtime_error
     */
    Core(Loop *loop, bool defaults);

    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     *  @param  settings    settings from the resolv.conf file
     * 
     *  @deprecated
     */
    Core(Loop *loop, const ResolvConf &settings);
    
    /**
     *  Destructor
     */
    virtual ~Core();

public:
    /**
     *  No copying
     *  @param  that
     */
    Core(const Core &that) = delete;
    
    /**
     *  Expose the event loop
     *  @return Loop
     */
    Loop *loop() { return _loop; }

    /**
     *  Create a fresh free query ID.
     *  @return a number guaranteed to be non-zero, uniformly random, and not already in use.
     */
    uint16_t generateUniqueQueryId();

    /**
     *  Free up this query ID.
     *  @param  id the query ID
     */
    void clearQueryId(uint16_t id) { _idsInUse.erase(id); }

    /**
     *  The send and receive buffer size 
     *  @return int32_t
     */
    int32_t buffersize() const { return _buffersize; }
    
    /**
     *  The period between sending the datagram again
     *  @return double
     */
    double interval() const { return _interval; }
    
    /**
     *  The time to wait for a response
     *  @return double
     */
    double timeout() const { return _timeout; }
    
    /**
     *  Max number of attempts / number of requests to send
     *  @return size_t
     */
    size_t attempts() const { return _attempts; }
    
    /**
     *  THe capacity: number of operations to run at the same time
     *  @return size_t
     */
    size_t capacity() const { return _capacity; }
    
    /**
     *  Default bits that are sent with each query
     *  @return Bits
     */
    const Bits &bits() const { return _bits; }

    /**
     *  Should all nameservers be rotated? otherwise they will be tried in-order
     *  @var bool
     */
    bool rotate() const { return _rotate; }

    /**
     *  Does a certain hostname exists in /etc/hosts? In that case a NXDOMAIN error should not be given
     *  @param  hostname        hostname to check
     *  @return bool            does it exists in /etc/hosts?
     */
    bool exists(const char *hostname) const { return _hosts.lookup(hostname) != nullptr; }

    /**
     *  Reschedule the timer
     *  @param  now         current time
     */
    void reschedule(double now);

    /**
     *  Expose the nameservers
     *  @return std::list<Nameserver>
     */
    std::list<Nameserver> &nameservers()
    {
        // expose the member
        return _nameservers;
    }
};

/**
 *  End of namespace
 */
}
