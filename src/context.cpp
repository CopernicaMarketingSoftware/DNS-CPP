/**
 *  Context.cpp
 * 
 *  Implementation file for the Context class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/context.h"
#include "job.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Do a dns lookup
 *  @param  name        the record name to look for
 *  @param  type        type of record (normally you ask for an 'a' record)
 *  @param  handler     object that will be notified when the query is ready
 *  @return Operation   object to interact with the operation while it is in progress
 */
Operation *Context::query(const char *domain, ns_type type, Handler *handler)
{
    // the request can throw (for example when the domain is invalid
    try
    {
        // we are going to create a self-destructing request
        return new Job(this, domain, type, handler);
    }
    catch (...)
    {
        // invalid parameters were supplied
        return nullptr;
    }
}

/**
 *  End of namespace
 */
}

