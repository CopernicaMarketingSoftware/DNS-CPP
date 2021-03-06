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
#include <deque>
#include <map>
#include <set>
#include "socket.h"
#include "monitor.h"
#include "connecting.h"

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
class Tcp : public Socket, private Monitor, private Connecting
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
     *  Size of the buffer to fill
     *  @var uint16_t
     */
    uint16_t _size;

    /**
     *  The response that is being filled right now
     *  @var vector
     */
    std::vector<unsigned char> _buffer;
    
    /**
     *  How many bytes of the frame were transferred?
     *  @var size_t
     */
    size_t _transferred = 0;

    /**
     *  Identifier user for monitoring the filedescriptor in the event loop
     *  @var void *
     */
    void *_identifier = nullptr;
    
    /**
     *  Is the socket already connected, or still in the process of being connected?
     *  @var enum
     */
    enum class State {
        connecting,     // busy setting up a connection
        failed,         // connection failure
        connected,      // connected
        lost,           // connection was lost in the middle of an operation
    } _state = State::connecting;

    /**
     *  Query IDs currently sent over the wire
     *  @var std::set
     */
    std::set<uint16_t> _queryids;

    /**
     *  Queries awaiting to be sent over the wire, but had an ID collision with one that is already in flight
     *  @var std::multimap
     */
    std::multimap<uint16_t, Query> _awaiting;

    /**
     *  Connectors that want to use this TCP socket for sending out a query
     *  @var std::deque
     */
    std::deque<Connector *> _connectors;
    
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
     *  Mark the connection as failed
     *  @param  state       the new state
     */
    void fail(const State &state);

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
     *  A response payload was received with this ID
     *  @param  id    The identifier
     */
    virtual void onReceivedId(uint16_t id) override;

    /**
     *  Number of bytes that we expect in the next read operation
     *  @return size_t
     */
    size_t expected() const;

    /**
     *  Check return value of a recv syscall
     *  @param  bytes       The bytes transferred
     *  @return boolean     True on success, false on failure (and we should leap out!)
     */
    bool updatetransferred(ssize_t bytes);
    
    /**
     *  Method that is called when there are no more subscribers, and that 
     *  is implemented in the derived classes. Watch out: this method can be called
     *  in the middle of loop through sockets so the implementation must be careful.
     */
    virtual void reset() override;

    /**
     *  Unsubscribe from the socket (in case a connector is no longer interested in the connect
     *  @param  connector   the object that unsubscribes
     */
    virtual void unsubscribe(Connector *connector) override;

    /**
     *  Blocking send this query
     *  @param  query  The query
     *  @return this or nullptr if something went wrong
     */
    Inbound *sendimpl(const Query &query);

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
     *  @param  connector       the object that subscribes
     *  @return Connecting      object that can be be used for unsubscribing
     */
    Connecting *subscribe(Connector *connector);

    /**
     *  The IP address to which this socket is connected
     *  @return Ip
     */
    const Ip &ip() const { return _ip; }

    /**
     *  Send a full query
     *  Note that this method can return nullptr in case the connection was already lost in the meantime
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
    virtual size_t process(size_t maxcalls) override;
};
    
/**
 *  End of namespace
 */
}
