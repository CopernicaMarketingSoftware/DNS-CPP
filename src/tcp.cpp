/**
 *  Tcp.cpp
 * 
 *  Implementation file for the Tcp class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/tcp.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/watcher.h"
#include "blocking.h"
#include "connector.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        user space event loop
 *  @param  ip          the IP to connect to
 *  @param  handler     parent object
 *  @throws std::runtime_error
 */
Tcp::Tcp(Loop *loop, const Ip &ip, Socket::Handler *handler) : 
    Socket(handler),
    _loop(loop), _ip(ip),
    _fd(socket(ip.version() == 6 ? AF_INET6 : AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
{
    // check if the socket was correctly created
    if (_fd < 0) throw std::runtime_error("failed to create socket");

    // we are going to set the nodelay flag to enforce that all data is immediately sent
    int nodelay = true;

    // set the option
    setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
    
    // connect the socket
    if (!connect(ip, 53)) { ::close(_fd); throw std::runtime_error("failed to connect"); }
    
    // now that we are sure to no longer throw, we can do the things that are cleanup in 
    // the destructor, start with monitoring the filedescriptor, we monitor for writability
    // to wait until the socket is connected
    _identifier = _loop->add(_fd, 2, this);
}
    
/**
 *  Destructor
 */
Tcp::~Tcp()
{
    // pass on to the loop
    if (_identifier) _loop->remove(_identifier, _fd, this);

    // close the socket
    ::close(_fd);
}

/**
 *  Helper function to make a connection
 *  @param  address     the address to connect to
 *  @param  size        size of the address structure
 *  @return bool
 */
bool Tcp::connect(struct sockaddr *address, size_t size)
{
    // make the connection
    auto result = ::connect(_fd, address, size);
    
    // check for success
    return result == 0 || errno == EINPROGRESS || errno == EWOULDBLOCK;
}

/**
 *  Helper function to make a connection
 *  @param  ip
 *  @param  port
 *  @return bool
 */
bool Tcp::connect(const Ip &ip, uint16_t port)
{
    // should we bind in the ipv4 or ipv6 fashion?
    if (ip.version() == 6)
    {
        // structure to initialize
        struct sockaddr_in6 info;

        // fill the members
        info.sin6_family = AF_INET6;
        info.sin6_port = htons(port);
        info.sin6_flowinfo = 0;
        info.sin6_scope_id = 0;

        // copy the address
        memcpy(&info.sin6_addr, (const struct in6_addr *)ip, sizeof(struct in6_addr));

        // connect
        return connect((struct sockaddr *)&info, sizeof(struct sockaddr_in6));
    }
    else
    {
        // structure to initialize
        struct sockaddr_in info;

        // fill the members
        info.sin_family = AF_INET;
        info.sin_port = htons(port);

        // copy the address
        memcpy(&info.sin_addr, (const struct in_addr *)ip, sizeof(struct in_addr));

        // connect
        return connect((struct sockaddr *)&info, sizeof(struct sockaddr_in));
    }
}


/**
 *  The error state of the socket -- this can be used to check whether the socket is 
 *  connected, only meaningful for non-blocking sockets
 *  @return int
 */
int Tcp::error() const
{
    // find out the socket error
    int error; unsigned int length = sizeof(int);
    
    // check the error
    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) return errno;

    // return the error
    return error;
}

/**
 *  Send a full query
 *  This is a blocking operation. Although it would have been better to make this
 *  non-blocking operation, it would make the code more complex and in reality we
 *  only send small messages over the socket, so we do not expect any buffering to
 *  take place anyway - so we have kept the code simple
 *  @param  query       the query to send
 *  @return Inbound     the object to which you can subscribe for responses
 */
Inbound *Tcp::send(const Query &query)
{
    // make the socket blocking
    Blocking blocking(_fd);
    
    // the first two bytes should contain the message size
    uint16_t size = htons(query.size());
    
    // send the size
    auto result1 = ::send(_fd, &size, sizeof(size), MSG_NOSIGNAL | MSG_MORE);
    
    // was this a success?
    if (result1 < 2) return nullptr;
    
    // now send the entire query
    auto result2 = ::send(_fd, query.data(), query.size(), MSG_NOSIGNAL);
    
    // report the result
    return result2 >= (ssize_t)query.size() ? this : nullptr;
}

/**
 *  Size of the response -- this method only works if we have already received the frist two bytes
 *  @return uint16_t
 */
uint16_t Tcp::responsesize() const
{
    // result variable
    uint16_t result;

    // get the first two bytes from the buffer
    memcpy(&result, _buffer.data(), 2);
    
    // put the bytes in the right order
    return ntohs(result);
}

/**
 *  Number of bytes that we expect in the next read operation
 *  @return size_t
 */
size_t Tcp::expected() const
{
    // if we have not yet received the first two bytes, we expect those first
    switch (_filled) {
    case 0:     return 2;
    case 1:     return 1;
    default:    return responsesize() - (_filled - 2);
    }
}

/**
 *  Reallocate the buffer if it turns out that our buffer is too small for the expected response
 *  @return bool
 */
bool Tcp::reallocate()
{
    // preferred buffer size
    size_t preferred = responsesize() + 2;
    
    // reallocate the buffer (but do not shrink)
    _buffer.resize(std::max(_buffer.size(), preferred));
    
    // report result
    return true;
}

/**
 *  Upgrate the socket from a _connecting_ socket to a _connected_ socket
 */
void Tcp::upgrade()
{
    // is the socket now connected?
    _connected = error() == 0;
    
    // if the connection failed
    if (!_connected) return fail();
    
    // already allocate enough data for the first two bytes (holding the size of a response)
    _buffer.resize(2);
    
    // we no longer monitor for writability, but for readability instead
    _loop->update(_identifier, _fd, 1, this);
    
    // In theory we should call '_handler->onBuffered()', which is conceptually more 
    // correct because it will trigger the lookups via the procedure in Core, and Core
    // can keep its internal counters of the number of running procedures up-to-date.
    // However, because we KNOW that _connector->onConnected() does not trigger any 
    // callbacks to user-space it is harmless and faster if we call that directly
    for (auto *connector : _connectors) connector->onConnected(_ip, this);
    
    // reset the connectors
    _connectors.clear();
}

/**
 *  Mark the connection as failed
 */
void Tcp::fail()
{
    // we can stop monitoring the socket
    _loop->remove(_identifier, _fd, this); 
    
    // reset the identifier
    _identifier = nullptr;
    
    // object is no longer connected
    _connected = false;

    // we connectors should be notified about the failure, but we postpone that so that
    // it can be triggered from the Core class via call to deliver() (so that Core can keep
    // all its timers up-to-date)
    _handler->onBuffered(this);
}

/**
 *  Method that is called when the socket becomes active (readable in our case)
 */
void Tcp::notify()
{
    // if the socket is not yet connected, it might be connected right now
    if (!_connected) return upgrade();
    
    // receive data from the socket
    auto result = ::recv(_fd, _buffer.data() + _filled, expected(), MSG_DONTWAIT);
    
    // do nothing if the operation is blocking
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
    
    // if there is a failure we leap out
    if (result <= 0) return fail();
    
    // update the number of bytes received
    _filled += result;
    
    // after we've received the first two bytes, we can reallocate the buffer so that it is of sufficient size
    if (_filled == 2 && !reallocate()) return fail();
    
    // continue waiting if we have not yet received everything there is
    if (expected() > 0) return;
    
    // all data has been received, buffer the response for now
    add(_ip, _buffer.data() + 2, _filled - 2);
    
    // for the next response we empty the buffer again
    _filled = 0;
}

/**
 *  Invoke callback handlers for buffered raw responses
 *  @param      watcher   The watcher to keep track if the parent object remains valid
 *  @param      maxcalls  The max number of callback handlers to invoke
 *  @return     number of callback handlers invoked
 */
size_t Tcp::deliver(size_t maxcalls)
{
    // the object could destruct in the meantime, because there are call to userspace
    Watcher watcher(this);
    
    // number of calls made
    size_t calls = 0;
    
    // is the object still busy connecting?
    bool connecting = !_connected && _identifier != nullptr;
    
    // inform them all
    while (calls < maxcalls && !_connectors.empty() && !connecting)
    {
        // get the oldest connector
        auto *connector = _connectors.front();
        
        // remove it from the vector
        _connectors.pop_front();
        
        // report the connection
        bool result = _connected ? connector->onConnected(_ip, this) : connector->onFailure(_ip);
        
        // update the counter
        if (result) calls += 1;
        
        // stop if userspace destructed us
        if (!watcher.valid()) return calls;
    }
    
    // if there are no more connectors (and also no subscribers) this connection can be removed
    if (_connectors.empty()) reset();
    
    // call base
    return calls + Socket::deliver(maxcalls - calls);
}

/**
 *  Subscribe to this socket (wait for the socket to be connected)
 *  @param  connector   the object that subscribes
 *  @return bool        as subscribing possible (not possible if connection failed)
 * 
 *  @todo   make an unsubscribe() counter-part
 */
bool Tcp::subscribe(Connector *connector)
{
    // this is not possible if the connection is already in a failed state
    if (!_connected && _identifier == nullptr) return false;
    
    // add the connector to be notified later when the connection is available
    _connectors.push_back(connector);
    
    // in case we're not yet connected we already wait for the connection to be ready,
    // and when we already have connectors, the parent was already informed
    if (!_connected || _connectors.size() > 1) return true;
    
    // notify the parent that there are actions to be performed
    _handler->onBuffered(this);
    
    // report success
    return true;
}

/**
 *  Method that is called when there are no more subscribers, and that 
 *  is implemented in the derived classes. Watch out: this method can be called
 *  in the middle of loop through sockets so the implementation must be careful.
 */
void Tcp::reset()
{
    // if we still have connectors or subscribers this should not do anything
    if (subscribers() > 0 || !_connectors.empty()) return;
    
    // there are no more subscribers, we are going to tell the parent about it
    auto *handler = (Tcp::Handler *)_handler;
    
    // notify our parent
    handler->onUnused(this);
}

/**
 *  End of namespace
 */
}

