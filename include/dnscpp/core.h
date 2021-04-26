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
#include "sockets.h"
#include "resolvconf.h"
#include "hosts.h"
#include "bits.h"
#include "now.h"
#include "lookup.h"
#include "processor.h"
#include "timer.h"
#include "intrusivequeue.h"
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
     *  The IP addresses of the servers that can be accessed
     *  @var std::vector<Ip>
     */
    std::vector<Ip> _nameservers;
    
    /**
     *  The contents of the /etc/hosts file
     *  @var Hosts
     */
    Hosts _hosts;

    /**
     *  All operations that are in progress.
     *  @var IntrusiveQueue<Lookup>
     */
    IntrusiveQueue<Lookup> _lookups;

    /**
     *  To avoid that external DNS servers, or our own response-buffer, is flooded
     *  with data, there is a limit on the number of operations that can run. If
     *  there are more operations than we can handle, this buffer is used for
     *  overflow (is not supposed to happen often!)
     *  @var std::deque<std::shared_ptr<Lookup>>
     */
    std::deque<std::shared_ptr<Lookup>> _scheduled;
    
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
    size_t _capacity = 1024;

    /**
     *  Calculate the delay until the next job
     *  @return double      the delay in seconds (or < 0 if there is no need to run a timer)
     */
    double delay(double now);

    /**
     *  Proceed with more operations
     *  @param  now
     */
    void proceed(double now);

    /**
     *  Invoke callback handlers of a lookup that's done.
     *  @param  watcher  The watcher to check if we're still valid
     *  @param  lookup   The lookup
     *  @return true if we should bail out immediately
     */
    bool finalize(const Watcher &watcher, std::shared_ptr<Lookup> &&lookup) const noexcept;

    /**
     *  Notify the timer that it expired
     */
    virtual void expire() override;
    
    /**
     *  Add a new lookup to the list
     *  @param  lookup
     *  @return Operation
     */
    Operation *add(std::shared_ptr<Lookup> lookup);
    
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

    /**
     *  Method that is called when a UDP socket has a buffer that it wants to deliver
     *  @param  sockets     the sockets with a buffer
     */
    void onBuffered(Sockets *sockets) override;

    /**
     *  Method that is called when a UDP socket has a buffer that it wants to deliver
     *  @param  now         current time
     */
    void reschedule(double now);

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
     *  Send a message over a UDP socket
     *  @param  ip              target IP
     *  @param  query           the query to send
     *  @return Inbound         the object that receives the answer
     */
    Inbound *datagram(const Ip &ip, const Query &query);

    /**
     *  Connect with TCP to a socket
     *  This is an async operation, the connection will later be passed to the connector
     *  @param  ip          IP address of the target nameservers
     *  @param  connector   the object interested in the connection
     *  @return bool
     */
    bool connect(const Ip &ip, std::shared_ptr<Connector> connector);

    /**
     *  An inflight lookup is done
     *  @param      lookup  The lookup
     */
    void done(std::shared_ptr<Lookup> lookup);

    /**
     *  Expose the nameservers
     *  @return std::list<Ip>
     */
    const std::vector<Ip> &nameservers() const { return _nameservers; }
};

/**
 *  End of namespace
 */
}
