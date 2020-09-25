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
 *  Method that is called when a raw response is received
 *  @param  query           the original query
 *  @param  response        the received response
 */
void Handler::onReceived(const Query &query, const Response &response)
{
    // check if the response was truncated (in which case the nameserver failed to handle tcp requests)
    if (response.truncated())
    {
        // @todo add a default implementation
        
        
    }
    else
    {
        // @todo add default implementation
    }
}
    
/**
 *  End of namespace
 */
}

