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
#include <arpa/nameser.h>
#include "core.h"
#include "reverse.h"

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
     *  @param  loop        your event loop
     */
    Context(Loop *loop) : Core(loop) {}
    
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
     *  Add a nameserver
     *  @param  ip
     */
    void nameserver(const Ip &ip)
    {
        // add to the member in the base class
        _nameservers.emplace_back(_loop, ip);
    }
    
    /**
     *  Do a dns lookup
     *  When you supply invalid parameters (for example a syntactivally invalid
     *  domain or an unsupported type) this method returns null.
     *  @param  name        the record name to look for
     *  @param  type        type of record (normally you ask for an 'a' record)
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const char *domain, ns_type type, Handler *handler);
    
    /**
     *  Do a reverse IP lookup, this is only meaningful for PTR lookups
     *  @param  ip          the ip address to lookup
     *  @param  handler     object that will be notified when the query is ready
     *  @return operation   object to interact with the operation while it is in progress
     */
    Operation *query(const Ip &ip, Handler *handler) { return query(Reverse(ip), ns_t_ptr, handler); }
};
    
/**
 *  End of namespace
 */
}

