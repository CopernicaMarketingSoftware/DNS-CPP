/**
 *  Connecting.h
 * 
 *  Base class of the TCP class. This base class offers limited functionality
 *  of the TCP class and is used internally to avoid that certain internal
 *  classes can already send out messages before a socket is fully connected
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
 *  Forward declarations
 */
class Connector;

/**
 *  Class definition
 */
class Connecting
{
protected:
    /**
     *  Constructor
     */
    Connecting() = default;

    /**
     *  Destructor
     */
    virtual ~Connecting() = default;

public:
    /**
     *  Unsubscribe from the socket 
     *  @param  connector
     */
    virtual void unsubscribe(Connector *connector) = 0;
};

/**
 *  End of namespace
 */
}

