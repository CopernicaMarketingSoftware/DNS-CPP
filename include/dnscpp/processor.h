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
     *  Method that is called when a dgram response is received
     *  @param  ip          the ip from where the response came (nameserver ip)
     *  @param  response    the received response
     *  @return bool
     */
    virtual bool onReceived(const Ip &ip, const Response &response) = 0;
};

/**
 *  End of namespace
 */
}
