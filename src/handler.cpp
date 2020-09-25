/**
 *  Handler.cpp
 * 
 *  Implementation file for the handler
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/handler.h"
#include "../include/dnscpp/response.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Method that is called when an operation times out.
 *  @param  operation       the operation that timed out
 *  @param  query           the query that was attempted
 */
void Handler::onTimeout(const Operation *operation, const Query &query)
{
    // we treat this as a server-failure
    onError(operation, query, ns_r_servfail);
}

/**
 *  Method that is called when a raw response is received
 *  @param  operation       the reporting operation
 *  @param  query           the original query
 *  @param  response        the received response
 */
void Handler::onResponse(const Operation *operation, const Query &query, const Response &response)
{
    // was there an error of any sort?
    if (response.rcode() != 0) return onError(operation, query, response.rcode());
    
    // if the message was truncated we also treat is as an error because from our perspective the server failed to respond
    if (response.truncated()) return onError(operation, query, ns_r_servfail);
    
    // we have a successful response
    onSuccess(operation, query, response);
}

/**
 *  Method that is called when a valid response was received.
 *  @param  operation       the operation that finished
 *  @param  query           the original query
 *  @param  response        the received response
 */
void Handler::onSuccess(const Operation *operation, const Query &query, const Response &response)
{
    // @todo inspect the response and call the most appropriate onReceived() method
}
    
/**
 *  End of namespace
 */
}

