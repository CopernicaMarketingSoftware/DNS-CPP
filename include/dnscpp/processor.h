/**
 *  Processor.h
 * 
 *  Object that is (internally!) implemented by classes that want to
 *  receive responses from UDP sockets.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Processor
{
public:
    /**
     *  Method that is called when a response is received
     *  @param  time        receive-time
     *  @param  address     the address of the nameserver from which it is received
     *  @param  response    the received response
     *  @param  size        size of the response
     */
    virtual void onReceived(time_t now, const struct sockaddr *addr, const unsigned char *response, size_t size) = 0;
};

/**
 *  End of namespace
 */
}
