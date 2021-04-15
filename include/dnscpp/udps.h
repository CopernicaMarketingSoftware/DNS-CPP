/**
 *  Udp.h
 *
 *  Internal class that implements a UDP socket over which messages
 *  can be sent to nameservers. You normally do not have to construct
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
#include "monitor.h"
#include "watchable.h"
#include "udp.h"
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

/**
 *  Class definition
 */
class Udps : private Watchable, Udp::Handler
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Method that is called when the udp socket has a buffer available
         *  @param  udp     the reporting socket
         */
        virtual void onBuffered(Udps *udp) = 0;
    };

private:
    /**
     *  The object that is interested in handling responses
     *  @var Handler*
     */
    Handler *_handler;

    /**
     *  Collection of all sockets
     *  @var std::list<Udp>
     */
    std::list<Udp> _sockets;

    /**
     *  The next socket to use for sending a new query
     *  @var size_t
     */
    std::list<Udp>::iterator _current;

    /**
     *  Close all sockets
     *  @todo: this method should disappear
     */
    void close();

    /**
     *  This method is called when a UDP socket has an inbound buffer that requires processing
     *  @param  udp     the reporting object
     */
    virtual void onBuffered(Udp *udp) override 
    { 
        // pass on to the handler
        _handler->onBuffered(this); 
    }
    
    /**
     *  Method that is called when an inbound socket is closed
     *  @param  udp     the reporting object
     */
    virtual void onClosed(Udp *udp) override;
    

public:
    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  handler     object that will receive all incoming responses
     *  @param  socketcount number of UDP sockets to keep open
     *  @throws std::runtime_error
     */
    Udps(Loop *loop, Handler *handler, size_t socketcount = 1);

    /**
     *  No copying
     *  @param  that
     */
    Udps(const Udps &that) = delete;
        
    /**
     *  Destructor
     */
    virtual ~Udps() = default;

    /**
     *  Send a query to the socket
     *  Watch out: you need to be consistent in calling this with either ipv4 or ipv6 addresses
     *  @param  ip          IP address of the target nameserver
     *  @param  query       the query to send
     *  @param  buffersize  strange parameter
     *  @return Inbound     the inbound object over which the message is sent
     *
     *  @todo   buffersize is strange
     */
    Inbound *send(const Ip &ip, const Query &query, int32_t buffersize);

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
    bool readable() const;

    /**
     *  Does one of the sockets have an inbound buffer (meaning: is there a backlog of unprocessed messages?)
     *  @return bool
     */
    bool buffered() const
    {
        // if there's a buffered response in one of the sockets then we consider ourselves buffered
        for (const auto &sock : _sockets) if (sock.buffered()) return true;

        // otherwise we're not buffered
        return false;
    }
};
    
/**
 *  End of namespace
 */
}
