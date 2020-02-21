/**
 *  Connection.h
 * 
 *  Class that is responsible for making a TCP connection to the
 *  nameserver, and to send the query to it, and to handle the response
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
#include "connector.h"
#include "receiver.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Connection : private Connector::Handler, private Receiver::Handler
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Called when the response has been received
         *  @param  connection
         *  @param  response
         */
        virtual void onReceived(Connection *connection, const Response &response) = 0;
        
        /**
         *  Called when the connection could not be used
         *  @param  connector
         */
        virtual void onFailure(Connection *connection) = 0;
    };

private:
    /**
     *  The actual TCP socket
     *  @var Tcp
     */
    Tcp _tcp;
    
    /**
     *  The initial procedure (setting up the connection) is the responsibility
     *  of the connector class
     *  @var Connector
     */
    Connector _connector;
    
    /**
     *  After the connector has done it's work, the responsibilty is
     *  handed over to the receiver that will do it's best to receive
     *  a response
     *  @var Receiver
     */
    Receiver _receiver;
    
    /**
     *  Reference to the query that should be sent over the connection
     *  @var Query
     */
    const Query &_query;
    
    /**
     *  The handler that will receive the response
     *  @var Handler
     */
    Handler *_handler;


    /**
     *  Called when the connection has been set up
     *  @param  connector
     *  @param  tcp
     */
    virtual void onConnected(Connector *connector, Tcp *tcp) override
    {
        // now that the socket is connected, we can send out the query, if
        // that succeeds we tell the receiver that he can start waiting for the response
        if (_tcp.send(_query)) _receiver.start();
        
        // the query could not be sent, report this as an error
        else _handler->onFailure(this);
    }
    
    /**
     *  Called when the connection could not be set up
     *  @param  connector
     *  @param  tcp
     */
    virtual void onFailure(Connector *connector, Tcp *tcp) override
    {
        // connection could not be set up, report this
        _handler->onFailure(this);
    }
    
    /**
     *  Method that is called when the response has been received
     *  @param  receiver        reporting object
     *  @param  response        the response data
     */
    virtual void onReceived(Receiver *receiver, const Response &response) override
    {
        // because we have only sent a single query, we can stop with receiving more
        _receiver.stop();

        // report to the parent
        _handler->onReceived(this, response);
    }
    
    /**
     *  Method that is called when the response was not received
     *  @param  receiver        reporting object
     */
    virtual void onFailure(Receiver *receiver) override
    {
        // error while receiving data, pass on
        _handler->onFailure(this);
    }

public:
    /**
     *  Constructor
     *  @param  loop        the event loop
     *  @param  ip          the IP address to connect to
     *  @param  query       the query to send over the connection
     *  @param  handler     parent object that is notified about the result
     */
    Connection(Loop *loop, const Ip &ip, const Query &query, Handler *handler) :
        _tcp(loop, ip),
        _connector(&_tcp, ip, this),
        _receiver(&_tcp, this),
        _query(query),
        _handler(handler) {}
        
    /**
     *  Destructor
     */
    virtual ~Connection() = default;
};

/**
 *  End of namespace
 */
}

