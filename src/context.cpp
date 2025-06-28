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

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Helper function to construct the configuration
 *  @param  defaults
 *  @return std::shared_ptr<Config>
 */
static std::shared_ptr<Config> createConfig(bool defaults)
{
    // if we do not have to load system defaults, we start with an empty config
    if (!defaults) return std::make_shared<Config>();
    
    // load the defaults from /etc/resolv.conf
    ResolvConf settings;
    
    // construct the context
    return std::make_shared<Config>(settings);
}    

/**
 *  Constructor
 *  You can specify whether the system defaults from /etc/resolv.conf and
 *  /etc/hosts should be loaded or not. If you decide to no load the system
 *  defaults, you must explicitly assign nameservers to the context before
 *  you can run any queries.
 *  @param  loop        your event loop
 *  @param  defaults    should system settings be loaded
 */
Context::Context(Loop *loop, bool defaults) : Context(loop, createConfig(defaults)) {}

/**
 *  Constructor
 *  @param  loop        your event loop
 *  @param  settings    settings parsed from the /etc/resolv.conf file
 * 
 *  @deprecated
 */
Context::Context(Loop *loop, const ResolvConf &settings) : Context(loop, std::make_shared<Config>(settings)) {}

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
    // pass to the core
    return _core->query(_config, domain, type, bits, handler);
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
    // pass to the core
    return _core->query(_config, ip, bits, handler);
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

