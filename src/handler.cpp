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
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/question.h"
#include "../include/dnscpp/handler.h"


/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Method that is called when an operation times out.
 *  @param  operation       the operation that timed out
 *  @param  query           the query that was attempted
 */
void Handler::onTimeout(const Operation *operation)
{
    // we treat this as a server-failure
    onFailure(operation, ns_r_servfail);
}

/**
 *  Method that is called when a raw response is received
 *  @param  operation       the reporting operation
 *  @param  response        the received response
 */
void Handler::onReceived(const Operation *operation, const Response &response)
{
    // was there an error of any sort?
    if (response.rcode() != 0) return onFailure(operation, response.rcode());
    
    // if the message was truncated we also treat is as an error because from our perspective the server failed to respond
    if (response.truncated()) return onFailure(operation, ns_r_servfail);
    
    // we have a successful response
    onResolved(operation, response);
}
    
/**
 *  End of namespace
 */
}

