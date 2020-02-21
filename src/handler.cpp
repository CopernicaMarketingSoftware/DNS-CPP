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

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Method that is called when a raw response is received
 *  @param  response        the received response
 */
void Handler::onReceived(const Response &response)
{
    // @todo add default implementation
}
    
/**
 *  End of namespace
 */
}

