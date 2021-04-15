/**
 *  Connector.h
 * 
 *  Class that is responsible for setting up a TCP connection
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "tcp.h"
#include "../include/dnscpp/monitor.h"

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Class definition
 */
class Connector : private Monitor
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Called when the connection has been set up
         *  @param  connector
         *  @param  tcp
         */
        virtual void onConnected(Connector *connector, Tcp *tcp) = 0;
        
        /**
         *  Called when the connection could not be set up
         *  @param  connector
         *  @param  tcp
         */
        virtual void onFailure(Connector *connector, Tcp *tcp) = 0;
    };
    
private:
    /**
     *  The socket that is being connected
     *  @var Tcp
     */
    Tcp *_tcp;

    /**
     *  Identifier of the monitor in the event loop
     *  @var void *
     */
    void *_identifier = nullptr;
    
    /**
     *  Pointer to the handler that is notified on success or failure
     *  @var Handler
     */
    Handler *_handler;


    /**
     *  Method from the monitor that is called by the event loop when the socket is writable
     */
    virtual void notify() override
    {
        // we no longer have to monitor
        _tcp->remove(_identifier, this);
        
        // forget the identifier
        _identifier = nullptr;
        
        // is the socket in an error-state?
        if (_tcp->error() == 0) _handler->onConnected(this, _tcp);
        
        // when there is an error, we report that
        else _handler->onFailure(this, _tcp);
    }


public:
    /**
     *  Constructor
     *  @param  socket      the socket socket
     *  @param  ip          the IP address to connect to
     *  @param  handler     object that is notified on success
     *  @throws std::runtime_error
     */
    Connector(Tcp *tcp, const Ip &ip, Handler *handler) : _tcp(tcp), _handler(handler)
    {
        // try to connect
        if (!tcp->connect(ip, 53)) throw std::runtime_error("failed to connect");
        
        // monitor the socket for writability, because that means that the socket is connected
        _identifier = tcp->monitor(2, this);
    }

    /**
     *  Destructor
     */
    virtual ~Connector()
    {
        // stop monitoring
        if (_identifier) _tcp->remove(_identifier, this);
    }
};
    
/**
 *  End of namespace
 */
}

