/**
 *  Response.h
 * 
 *  Class to parse a response from a nameserver and to extract the
 *  individual properties from it
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
#include "message.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Response : public Message 
{
public:
    /**
     *  Constructor
     *  @param  buffer      the raw data that we received
     *  @param  size        size of the buffer
     *  @throws std::runtime_error
     */
    Response(const unsigned char *buffer, size_t size) : Message(buffer, size) {}
    
    /**
     *  Copy constructor
     *  It is better not to use this too often as it might not be too efficient to copy responses around
     *  @param  that
     *  @throws std::runtime_error
     */
    Response(const Response &that) : Message(that) {}
};
    
/**
 *  End of namespace
 */
}
