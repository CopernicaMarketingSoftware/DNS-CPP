/**
 *  Context.h
 *
 *  Main context for DNS lookups. This is the starting point
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
#include <vector>
#include "handler.h"
#include "type.h"
#include "core.h"
#include "callbacks.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Handler;
class Operation;
class SearchLookupHandler;

/**
 *  Class definition
 */
class Context : private Core
{
private:
    /**
     *  The configuration used by this context
     *  @var std::shared<Config>
     */
    std::shared_ptr<Config> _config;

    /**
     *  Default bits to include in queries
     *  @var Bits
     */
    Bits _bits;

    /**
     *  Should the search path be respected?
     *  @param  domain      the domain to lookup
     *  @param  handler     handler that is already in use
     *  @return bool
     */
    bool searchable(const char *domain, DNS::Handler *handler) const;

    /**
     *  Constructor
     *  @param  loop
     *  @param  config
     */
    Context(Loop *loop, const std::shared_ptr<Config> &config) : Core(loop, config), _config(config) {}

public:
    /**
     *  Constructor
     *  You can specify whether the system defaults from /etc/resolv.conf and
     *  /etc/hosts should be loaded or not. If you decide to no load the system
     *  defaults, you must explicitly assign nameservers to the context before
     *  you can run any queries.
     *  @param  loop        your event loop
     *  @param  defaults    should system settings be loaded
     */
    Context(Loop *loop, bool defaults = true);

    /**
     *  Constructor
     *  @param  loop        your event loop
     *  @param  settings    settings parsed from the /etc/resolv.conf file
     * 
     *  @deprecated
     */
    Context(Loop *loop, const ResolvConf &settings);
    
    /**
     *  No copying
     *  @param  that
     */
    Context(const Context &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Context() = default;
    
    /**
     *  Clear the list of nameservers
     */
    void clear()
    {
        // empty the list
        _config->clear();
    }
    
    /**
     *  Add a nameserver
     *  @param  ip
     */
    void nameserver(const Ip &ip)
    {
        // add to the member in the base class
        _config->nameserver(ip);
    }
    
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
     *  Set max time to wait for a response
     *  @param timeout      time in seconds
     */
    void timeout(double timeout) { _config->timeout(timeout); }
    
    /**
     *  Interval before a datagram is sent again
     *  @return double
     */
    double interval() const { return _config->interval(); }
    
    /**
     *  Set interval before a datagram is sent again
     *  @param  interval    time in seconds
     */
    void interval(double interval) { _config->interval(interval); }

    /**
     *  Max number of attempts / requests to send per query
     *  @return size_t
     */
    size_t attempts() const { return _config->attempts(); }
    
    /**
     *  Set the max number of attempts
     *  @param  attempt     max number of attemps
     */
    void attempts(size_t attempts) { _config->attempts(attempts); }

    /**
     *  Set the send & receive buffer size of each individual UDP socket
     *  @param value  the value to set
     */
    void buffersize(int32_t value);

    /**
     *  Set the capacity: number of operations to run at the same time
     *  @param  value       the new value
     */
    void capacity(size_t value);

    /**
     *  Default bits that are sent with each query
     *  @return Bits
     */
    const Bits &bits() const { return _bits; }
    
    /**
     *  Enable or disable certain bits
     *  @param  value
     */
    void bits(const Bits &bits) { _bits = bits; }
    void enable(const Bits &bits) { _bits.enable(bits); }
    void disable(const Bits &bits) { _bits.disable(bits); }

    /**
     *  Should all nameservers be rotated? otherwise they will be tried in-order
     *  @return bool
     */
    bool rotate() const { return _config->rotate(); }

    /**
     *  Set the rotate setting: If true, nameservers will be rotated, if false, nameservers are tried in-order
     *  @param rotate   the new setting
     */
    void rotate(bool rotate) { _config->rotate(rotate); }
    
    /**
     *  Set the max number of calls that are made to userspace in one iteration
     *  @param  value       the new value
     */
    void maxcalls(size_t value) { _maxcalls = value; }

    /**
     *  The 'ndots' setting from resolv.conf
     *  @return ndots
     */
    uint8_t ndots() const { return _config->ndots(); }
    
    /**
     *  Change the ndots setting
     *  @param  value       the new value
     */
    void ndots(uint8_t value) { _config->ndots(value); }
    
    /**
     *  Do a dns lookup and pass the result to a user-space handler object
     *  When you supply invalid parameters (for example a syntactivally invalid
     *  domain or an unsupported type) this method returns null.
     *  @param  name        the record name to look for
     *  @param  type        type of record (normally you ask for an 'a' record)
     *  @param  bits        bits to include in the query
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler);
    Operation *query(const char *domain, ns_type type, DNS::Handler *handler) { return query(domain, type, _bits, handler); }

    /**
     *  Do a reverse IP lookup, this is only meaningful for PTR lookups
     *  @param  ip          the ip address to lookup
     *  @param  bits        bits to include in the query
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const Ip &ip, const Bits &bits, DNS::Handler *handler);
    Operation *query(const Ip &ip, DNS::Handler *handler) { return query(ip, _bits, handler); }
    
    /**
     *  Do a dns lookup and pass the result to callbacks
     *  @param  name        the record name to look for
     *  @param  type        type of record (normally you ask for an 'a' record)
     *  @param  bits        bits to include in the query
     *  @param  success     function that will be called on success
     *  @param  failure     function that will be called on failure
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const char *domain, ns_type type, const Bits &bits, const SuccessCallback &success, const FailureCallback &failure);
    Operation *query(const char *domain, ns_type type, const SuccessCallback &success, const FailureCallback &failure) { return query(domain, type, _bits, success, failure); }

    /**
     *  Do a reverse dns lookup and pass the result to callbacks
     *  @param  ip          the ip address to lookup
     *  @param  bits        bits to include in the query
     *  @param  success     function that will be called on success
     *  @param  failure     function that will be called on failure
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const DNS::Ip &ip, const Bits &bits, const SuccessCallback &success, const FailureCallback &failure);
    Operation *query(const DNS::Ip &ip, const SuccessCallback &success, const FailureCallback &failure) { return query(ip, _bits, success, failure); }
    
    /**
     *  Expose some getters from core
     */
    using Core::capacity;
};
    
/**
 *  End of namespace
 */
}

