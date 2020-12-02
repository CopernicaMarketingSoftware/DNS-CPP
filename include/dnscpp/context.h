/**
 *  Context.h
 *
 *  Main context for DNS lookups. This is the starting point
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <vector>
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

/**
 *  Class definition
 */
class Context : private Core
{
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
    Context(Loop *loop, bool defaults = true) : Core(loop, defaults) {}

    /**
     *  Constructor
     *  @param  loop        your event loop
     *  @param  settings    settings parsed from the /etc/resolv.conf file
     * 
     *  @deprecated
     */
    Context(Loop *loop, const ResolvConf &settings) : Core(loop, settings) {}
    
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
        _nameservers.clear();
    }
    
    /**
     *  Add a nameserver
     *  @param  ip
     */
    void nameserver(const Ip &ip)
    {
        // add to the member in the base class
        _nameservers.emplace_back(static_cast<Core*>(this), ip);
    }

    /**
     *  The send and receive buffer size 
     *  @param  size      the requested buffer size in bytes, or default with 0. 
     *                    only gets applied to new sockets.
     */
    void buffersize(int32_t size) 
    {
        // store the property
        _buffersize = size;
    }
    
    /**
     *  Set max time that a request may last in seconds.
     *  @param expire       time in seconds. default is 60.0, minimum is 0.1
     */
    void expire(double expire)
    {
        // store property, make sure the numbers are reasonably clamped
        _expire = std::max(expire, 0.1);
    }
    
    /**
     *  Set interval in seconds to wait before contacting the next server.
     *  @param  spread      time in seconds. default is 0.1, minimum is 0.0.
     */
    void spread(double spread)
    {
        // store property, make sure the numbers are reasonably clamped
        _spread = std::max(spread, 0.0);
    }
    
    /**
     *  Set interval before a datagram is repeated to the same server, in seconds.
     *  @param  interval    time in seconds. default is 2.0, minimum is 0.1.
     */
    void interval(double interval)
    {
        // store property, make sure the numbers are reasonably clamped
        _interval = std::max(interval, 0.1);
    }
    
    /**
     *  Enable or disable certain bits
     *  @param  value
     */
    void bits(const Bits &bits) { _bits = bits; }
    
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
    Operation *query(const char *domain, ns_type type, const Bits &bits, Handler *handler);
    Operation *query(const char *domain, ns_type type, Handler *handler) { return query(domain, type, _bits, handler); }
    
    /**
     *  Do a reverse IP lookup, this is only meaningful for PTR lookups
     *  @param  ip          the ip address to lookup
     *  @param  bits        bits to include in the query
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const Ip &ip, const Bits &bits, Handler *handler);
    Operation *query(const Ip &ip, Handler *handler) { return query(ip, _bits, handler); }
    
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
    using Core::buffersize;
    using Core::bits;
    using Core::rotate;
    using Core::spread;
    using Core::expire;
    using Core::interval;
};
    
/**
 *  End of namespace
 */
}

