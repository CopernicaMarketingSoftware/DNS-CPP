/**
 *  Context.cpp
 * 
 *  Implementation file for the Context class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2025 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/context.h"
#include "remotelookup.h"
#include "locallookup.h"
#include "idgenerator.h"
#include "searchlookup.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Set the send & receive buffer size of each individual UDP socket
 *  @param value  the value to set
 */
void Context::buffersize(int32_t value)
{
    // pass to the actual sockets
    _ipv4.buffersize(value);
    _ipv6.buffersize(value);
}

/**
 *  Set the capacity: number of operations to run at the same time
 *  @param  value       the new value
 */
void Context::capacity(size_t value)
{
    // store property
    _capacity = std::min((size_t)IdGenerator::capacity(), std::max(size_t(1), value));
}

/**
 *  Should the search path be respected?
 *  @param  domain      the domain to lookup
 *  @param  handler     handler that is already in use
 *  @return bool
 */
bool Context::searchable(const char *domain, DNS::Handler *handler) const
{
    // length of the lookup
    size_t length = strlen(domain);
    
    // empty domains fail anyway
    if (length == 0) return false;
    
    // canonical domains don't go into recursion
    if (domain[length-1] == '.') return false;
    
    // count the dots
    size_t ndots = std::count(domain, domain + length + 1, '.');
    
    // compare with the 'ndots' setting
    if (ndots >= _ndots) return false;
    
    // do not do recursion (if the current handler already is a SearchLookup)
    return dynamic_cast<SearchLookup*>(handler) == nullptr;
}

/**
 *  Do a dns lookup
 *  @param  domain      the record name to look for
 *  @param  type        type of record (normally you ask for an 'a' record)
 *  @param  bits        bits to include in the query
 *  @param  handler     object that will be notified when the query is ready
 *  @return Operation   object to interact with the operation while it is in progress
 */
Operation *Context::query(const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler)
{
    // check if we should respect the search path
    if (searchable(domain, handler)) return new SearchLookup(this, _authority, type, bits, domain, handler);
    
    // for A and AAAA lookups we also check the /etc/hosts file
    if (type == ns_t_a    && _authority->hosts().lookup(domain, 4)) return add(new LocalLookup(this, _authority, domain, type, handler));
    if (type == ns_t_aaaa && _authority->hosts().lookup(domain, 6)) return add(new LocalLookup(this, _authority, domain, type, handler));
    
    // the request can throw (for example when the domain is invalid
    try
    {
        // we are going to create a self-destructing request
        return add(new RemoteLookup(this, _authority, domain, type, bits, handler));
    }
    catch (...)
    {
        // invalid parameters were supplied
        return nullptr;
    }
}



/**
 *  Do a reverse IP lookup, this is only meaningful for PTR lookups
 *  @param  ip          the ip address to lookup
 *  @param  bits        bits to include in the query
 *  @param  handler     object that will be notified when the query is ready
 *  @return operation   object to interact with the operation while it is in progress
 */
Operation *Context::query(const Ip &ip, const Bits &bits, DNS::Handler *handler)
{
    // if the /etc/hosts file already holds a record
    if (_authority->hosts().lookup(ip)) return add(new LocalLookup(this, _authority, ip, handler));

    // pass on to the regular query method
    return query(Reverse(ip), TYPE_PTR, bits, handler);
}

/**
 *  Do a dns lookup and pass the result to callbacks
 *  @param  name        the record name to look for
 *  @param  type        type of record (normally you ask for an 'a' record)
 *  @param  bits        bits to include in the query
 *  @param  success     function that will be called on success
 *  @param  failure     function that will be called on failure
 *  @return operation   object to interact with the operation while it is in progress
 */
Operation *Context::query(const char *domain, ns_type type, const Bits &bits, const SuccessCallback &success, const FailureCallback &failure)
{
    // use a self-destructing wrapper for the handler
    return query(domain, type, bits, new Callbacks(success, failure));
}

/**
 *  Do a reverse dns lookup and pass the result to callbacks
 *  @param  ip          the ip address to lookup
 *  @param  bits        bits to include in the query
 *  @param  success     function that will be called on success
 *  @param  failure     function that will be called on failure
 *  @return operation   object to interact with the operation while it is in progress
 */
Operation *Context::query(const DNS::Ip &ip, const Bits &bits, const SuccessCallback &success, const FailureCallback &failure)
{
    // use a self-destructing wrapper for the handler
    return query(ip, bits, new Callbacks(success, failure));
}

/**
 *  End of namespace
 */
}

