/**
 *  Udp.h
 *
 *  Internal class that implements a UDP socket over which messages
 *  can be sent to nameservers. You normally do not have to construct
 *  this class in user space, it is used internally by the Context class.
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
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <sys/socket.h>
#include "monitor.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Query;
class Loop;
class Ip;
class Response;

/**
 *  Class definition
 */
class Udp : private Monitor
{
public:
    /**
     *  Interface that can be implemented by listeners
     */
    class Handler
    {
    public:
        /**
         *  Method that is called when a response is received
         *  @param  ip          the ip of the nameserver from which it is received
         *  @param  response    the received response
         */
        virtual void onReceived(const Ip &ip, const Response &response) = 0;
    };
    
private:
    /**
     *  The event loop in user space
     *  @var Loop
     */
    Loop *_loop;
    
    /**
     *  The filedescriptor of the socket
     *  @var int
     */
    int _fd;
    
    /**
     *  User space identifier of this monitor
     *  @var void *
     */
    void *_identifier;
    
    /**
     *  The object that is interested in handling responses
     *  @var Handler
     */
    Handler *_handler;
    
    /**
     *  Method that is called from user-space when the socket becomes
     *  readable.
     */
    virtual void notify() override;
    
    /**
     *  Send a query to a certain nameserver
     *  @param  address     target address
     *  @param  size        size of the address
     *  @param  query       query to send
     *  @return bool
     */
    bool send(const struct sockaddr *address, size_t size, const Query &query);

public:
    /**
     *  Constructor
     *  @param  loop        event loop in user space
     *  @param  version     IP version (ipv4 or ipv6)
     *  @param  handler     object that will receive all incoming responses
     *  @throws std::runtime_error
     */
    Udp(Loop *loop, int version, Handler *handler);
    
    /**
     *  No copying
     *  @param  that
     */
    Udp(const Udp &that) = delete;
        
    /**
     *  Destructor
     */
    virtual ~Udp();

    /**
     *  Send a query to a nameserver
     *  @param  ip      IP address of the nameserver
     *  @param  query   the query to send
     *  @return bool
     */
    bool send(const Ip &ip, const Query &query);
};
    
/**
 *  End of namespace
 */
}
