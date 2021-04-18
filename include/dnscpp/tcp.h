/**
 *  Tcp.h
 * 
 *  Class that takes care of setting up a tcp connection and that waits
 *  for a response
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Depdencies
 */
#include <netinet/tcp.h>
#include <unistd.h>
#include "socket.h"
#include "monitor.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Loop;
class Query;
class Connector;
    
/**
 *  Class definition
 */
class Tcp : public Socket, private Monitor
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler : public Socket::Handler
    {
    public:
        /**
         *  Method that is to be called when the socket is no longer in use (there are no more subscribers)
         *  @param  socket
         */
        virtual void onUnused(Tcp *tcp) = 0;
    };

private:
    /**
     *  The event loop to which the filedescriptor is bound
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  The target IP address
     *  @var Ip
     */
    const Ip _ip;

    /**
     *  The filedescriptor
     *  @var int
     */
    int _fd;
    
    /**
     *  The buffer that is being filled right now (first two bytes contain the size)
     *  @var unsigned char *
     */
    unsigned char *_buffer = nullptr;
    
    /**
     *  Capacity of the buffer (how much memory is allocated)
     *  @var size_t
     */
    size_t _capacity = 0;
    
    /**
     *  Size of the buffer (how much of the capacity is filled?)
     *  @var size_t
     */
    size_t _filled = 0;
    
    /**
     *  Identifier user for monitoring the filedescriptor in the event loop
     *  @var void *
     */
    void *_identifier = nullptr;
    
    /**
     *  Is the socket already connected, or still in the process of being connected?
     *  @var bool
     */
    bool _connected = false;

    /**
     *  Connectors that want to use this TCP socket for sending out a query
     *  @var std::vector
     */
    std::vector<Connector *> _connectors;
    
    /**
     *  Helper function to make a connection
     *  @param  address     the address to connect to
     *  @param  size        size of the address structure
     *  @return bool
     */
    bool connect(struct sockaddr *address, size_t size);
    
    /**
     *  Helper function to make a connection
     *  @param  ip
     *  @param  port
     *  @return bool
     */
    bool connect(const Ip &ip, uint16_t port);

    /**
     *  Upgrate the socket from a _connecting_ socket to a _connected_ socket
     */
    void upgrade();

    /**
     *  The error state of the socket -- this can be used to check whether the socket is 
     *  connected. Meaningful for non-blocking sockets.
     *  @return int
     */
    int error() const;

    /**
     *  Method that is called when the socket becomes active (readable in our case)
     */
    virtual void notify() override;

    /**
     *  Reallocate the buffer if it turns out that our buffer is too small for the expected response
     *  @return bool
     */
    bool reallocate();

    /**
     *  Size of the response -- this method only works if we have already received the frist two bytes
     *  @return uint16_t
     */
    uint16_t responsesize() const;
    
    /**
     *  Number of bytes that we expect in the next read operation
     *  @return size_t
     */
    size_t expected() const;
    
    /**
     *  Method that is called when there are no more subscribers, and that 
     *  is implemented in the derived classes. Watch out: this method can be called
     *  in the middle of loop through sockets so the implementation must be careful.
     */
    virtual void reset() override;

public:
    /**
     *  Constructor
     *  @param  loop        user space event loop
     *  @param  ip          the IP to connect to
     *  @param  handler     parent object
     *  @throws std::runtime_error
     */
    Tcp(Loop *loop, const Ip &ip, Socket::Handler *handler);
    
    /**
     *  Destructor
     */
    virtual ~Tcp();
    
    /**
     *  Subscribe to this socket (wait for the socket to be connected)
     *  @param  connector   the object that subscribes
     *  @return bool        was it possible to subscribe (not possible in failed state)
     */
    bool subscribe(Connector *connector);

    /**
     *  The IP address to which this socket is connected
     *  @return Ip
     */
    const Ip &ip() const { return _ip; }

    /**
     *  Send a full query
     *  @param  query       the query to send
     *  @return Inbound     the object that can be subscribed to for further processing
     */
    Inbound *send(const Query &query);

    /**
     *  Extended deliver() method (derives from the base class) that also takes the responsibility
     *  of passing the connection to the connectors.
     *  @param  maxcalls    max number of calls to make
     *  @return size_t
     */
    virtual size_t deliver(size_t maxcalls) override;
};
    
/**
 *  End of namespace
 */
}

