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
 * 
 *  This normally happens when none of the nameservers send back a response.
 * 
 *  @param  operation       the operation that timed out
 *  @param  query           the query that was attempted
 */
void Handler::onTimeout(const Operation *operation, const Query &query)
{
    // @todo add handling to report an error
    
    
}

/**
 *  Method that is called when a raw response is received
 *  @param  operation       the reporting operation
 *  @param  query           the original query
 *  @param  response        the received response
 */
void Handler::onReceived(const Operation *operation, const Query &query, const Response &response)
{
    // check if the response was truncated (in which case the nameserver failed to handle tcp requests)
    if (response.truncated())
    {
        // @todo add handling to report an error
        
        
    }
    else
    {
        // @todo add handling to report success
    }
}
    
/**
 *  End of namespace
 */
}

