/**
 *  Sockets.h
 *
 *  Internal class that bundles all the socket over which messages
 *  are sent to nameservers. You normally do not have to construct
 *  this class in user space, it is used internally by the Context class.
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
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <sys/socket.h>
#include <memory>
#include "monitor.h"
#include "watchable.h"
#include "udp.h"
#include "tcp.h"
#include <list>
#include <string>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Core;
class Query;
class Loop;
class Ip;
class Response;
class Processor;
class Connector;

/**
 *  Class definition
 */
class Sockets : private Watchable, private Tcp::Handler
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Method that is called when one of the sockets has a buffer of received messages
         *  @param  udp     the reporting socket
         */
        virtual void onActive(Sockets *udp) = 0;
    };
    
private:
    /**
     *  The main event loop
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  The object that is interested in handling responses
     *  @var Handler*
     */
    Handler *_handler;

    /**
     *  Collection of all UDP sockets
     *  @var std::list<Udp>
     */
    std::list<Udp> _udps;

    /**
     *  The next UDP socket to use for sending a new query
     *  @var size_t
     */
    std::list<Udp>::iterator _current;

    /**
     *  Collection of TCP sockets
     *  @var std::vector
     */
    std::vector<std::shared_ptr<Tcp>> _tcps;

    /**
     *  This method is called when a socket has an inbound buffer that requires processing
     *  @param  socket  the reporting object
     */
    virtual void onActive(Socket *socket) override 
    { 
        // pass on to the handler
        _handler->onActive(this); 
    }

    /**
     *  Method that is to be called when the socket is no longer in use (there are no more subscribers)
     *  @param  socket
     */
    virtual void onUnused(Tcp *tcp) override;
    
    
public:
    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  handler     object that will receive all incoming responses
     *  @throws std::runtime_error
     */
    Sockets(Loop *loop, Handler *handler);

    /**
     *  No copying
     *  @param  that
     */
    Sockets(const Sockets &that) = delete;
        
    /**
     *  Destructor
     */
    virtual ~Sockets() = default;

    /**
     *  Update the number of sockets
     *  Watch out: it is only possible to _increase_ the number of sockets
     *  This is useful to spread out the load over multiple sockets (especially for programs with   
     *  many DNS lookups, where new lookups are started before previous lookups are completed)
     *  @param  value       new max value
     */
    void sockets(size_t count);

    /**
     *  Send a query to the socket
     *  Watch out: you need to be consistent in calling this with either ipv4 or ipv6 addresses
     *  @param  ip          IP address of the target nameserver
     *  @param  query       the query to send
     *  @return Inbound     the inbound object over which the message is sent
     */
    Inbound *datagram(const Ip &ip, const Query &query);

    /**
     *  Connect with TCP to a socket
     *  This is an async operation, the connection will later be passed to the connector
     *  @param  ip          IP address of the target nameservers
     *  @param  connector   the object interested in the connection
     *  @return Connecting  the socket to which we're busy to connect
     */
    Connecting *connect(const Ip &ip, Connector *connector);

    /**
     *  Deliver messages that have already been received and buffered to their appropriate processor
     *  @param  size_t      max number of calls to userspace
     *  @return size_t      number of processed answers
     */
    size_t deliver(size_t maxcalls);

    /**
     *  Is the socket now readable?
     *  @return bool
     */
    bool readable2() const;

    /**
     *  The inbound buffersize for new sockets
     *  @param  size        the new buffer size
     */
    void buffersize(size_t size)
    {
        // pass on
        for (auto &socket: _udps) socket.buffersize(size);
    }

    /**
     *  Does one of the sockets have an inbound buffer (meaning: is there a backlog of unprocessed messages?)
     *  @return bool
     */
    bool active() const
    {
        // if there's a buffered response in one of the sockets then we consider ourselves buffered
        for (const auto &sock : _udps) if (sock.active()) return true;

        // otherwise we're not buffered
        return false;
    }
};

/**
 *  End of namespace
 */
}
