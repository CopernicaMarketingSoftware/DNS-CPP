/**
 *  Connector.h
 * 
 *  Interface that is implemented by class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
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
 *  Forward declarations
 */
class Ip;
class Tcp;

/**
 *  Class definition
 */
class Connector
{
public:
    /**
     *  Called when the connection has been set up
     *  @param  ip          ip to which a connection was set up
     *  @param  tcp         the actual TCP connection
     *  @return bool        was there a call back to userspace
     */
    virtual bool onConnected(const Ip &ip, Tcp *tcp) = 0;
    
    /**
     *  Called when the connection could not be set up
     *  @param  ip          ip to which a connection was set up
     *  @return bool        was there a call back to userspace?
     */
    virtual bool onFailure(const Ip &ip) = 0;
};
    
/**
 *  End of namespace
 */
}

