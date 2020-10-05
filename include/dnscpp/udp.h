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
         *  @param  size        size of the response
         */
        virtual void onReceived(const Ip &ip, const unsigned char *response, size_t size) = 0;
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
    int _fd = -1;
    
    /**
     *  User space identifier of this monitor
     *  @var void *
     */
    void *_identifier = nullptr;
    
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

    /**
     *  Open the socket (this is optional, the socket is automatically opened when you start sending to it)
     *  @param  version
     *  @return bool
     */
    bool open(int version);

public:
    /**
     *  Constructor
     *  @param  loop        event loop in user space
     *  @param  handler     object that will receive all incoming responses
     *  @throws std::runtime_error
     */
    Udp(Loop *loop, Handler *handler);
    
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
     *  Close the socket (this is useful if you do not expect incoming data anymore)
     *  The socket will be automatically opened if you start sending to it
     *  @return bool
     */
    bool close();

    /**
     *  Send a query to the socket
     *  Watch out: you need to be consistent in calling this with either ipv4 or ipv6 addresses
     *  @param  ip      IP address of the target nameserver
     *  @param  query   the query to send
     *  @return bool
     */
    bool send(const Ip &ip, const Query &query);
};
    
/**
 *  End of namespace
 */
}
