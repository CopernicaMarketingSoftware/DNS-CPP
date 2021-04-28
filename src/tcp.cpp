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
#include "../include/dnscpp/processor.h"
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
    // @todo see https://tools.ietf.org/html/rfc7766#section-7
    // We MUST avoid having duplicate ID's that are active on the same connection,
    // for example by delaying queries with identical ids until earlier queries are ready
    
    // if the connection was already lost in the meantime
    if (_state != State::connected) return nullptr;
    
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
 *  Number of bytes that we expect in the next read operation
 *  @return size_t
 */
size_t Tcp::expected() const
{
    // if we have not yet received the first two bytes, we expect those first
    switch (_transferred) {
    case 0:     return sizeof(uint16_t);
    case 1:     return sizeof(uint16_t) - 1;
    default:    return _size - (_transferred - sizeof(uint16_t));
    }
}

/**
 *  Upgrate the socket from a _connecting_ socket to a _connected_ socket
 */
void Tcp::upgrade()
{
    // is the socket now connected?
    if (error() != 0) return fail(State::failed);
    
    // the connection succeeded
    _state = State::connected;

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
 *  @param  state       the new state
 */
void Tcp::fail(const State &state)
{
    // we can stop monitoring the socket
    _loop->remove(_identifier, _fd, this); 
    
    // reset the identifier
    _identifier = nullptr;
    
    // update the state
    _state = state;

    // the connectors should be notified about the failure, but we postpone that so that
    // it can be triggered from the Core class via call to process() (so that Core can keep
    // all its timers up-to-date)
    _handler->onActive(this);
}

/**
 *  Check return value of a recv syscall
 *  @param  bytes       The bytes transferred
 *  @return boolean     True on success, false on failure (and we should leap out!)
 */
bool Tcp::updatetransferred(ssize_t result)
{
    // the operation can block, this is ok
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;

    // if there is a failure we leap out as well
    if (result <= 0) return fail(State::lost), false;

    // update the number of transferred bytes
    _transferred += result;

    // don't leap out
    return true;
}

/**
 *  Method that is called when the socket becomes active (readable in our case)
 */
void Tcp::notify()
{
    // @todo see https://tools.ietf.org/html/rfc7766#section-6.2.4
    // if we lose the connection while we have not yet received all responses, 
    // we SHOULD retry the TCP requests
    
    // if the socket is not yet connected, it might be connected right now
    if (_state == State::connecting) return upgrade();

    // We can be in two receive states: the first state is that we're waiting for the
    // size of the buffer. The second state is that we are waiting for the response content itself.
    // To determine in what state we're in, we can check how many bytes have been transferred.
    // If that's less than 2 then we're still waiting for the response size.
    if (_transferred < sizeof(uint16_t))
    {
        // read one or two bytes into the _size variable
        const auto result = ::recv(_fd, (uint8_t*)&_size + _transferred, sizeof(uint16_t) - _transferred, MSG_DONTWAIT);

        // if there is a failure we leap out
        if (updatetransferred(result)) return;

        // if we still haven't received the two bytes we should leap out here
        if (_transferred < sizeof(uint16_t)) return;

        // OK: the size of the rest of the frame was received, we know how much to allocate
        // update the size
        _size = ntohs(_size);

        // size the buffer accordingly
        _buffer.resize(_size);
    }

    // calculate offset into the buffer
    size_t offset = _transferred - sizeof(uint16_t);

    // This is the second state of the Tcp state machine. At this point we know we have
    // received at least two bytes of the frame, and so we know we have resized the
    // buffer accordingly. All that's left to do is to await the full response content
    if (!updatetransferred(::recv(_fd, _buffer.data() + offset, _buffer.size() - offset, MSG_DONTWAIT))) return;

    // continue waiting if we have not yet received everything there is
    if (expected() > 0) return;

    // all data has been received, we can move the response content into a deferred list to be processed later
    add(_ip, move(_buffer));
    
    // for the next response we empty the buffer again
    _transferred = 0;
}

/**
 *  Invoke callback handlers for buffered raw responses
 *  @param      watcher   The watcher to keep track if the parent object remains valid
 *  @param      maxcalls  The max number of callback handlers to invoke
 *  @return     number of callback handlers invoked
 */
size_t Tcp::process(size_t maxcalls)
{
    // the object could destruct in the meantime, because there are call to userspace
    Watcher watcher(this);
    
    // number of calls made
    size_t calls = 0;
    
    // inform all connectors that the socket became connected or that the connection failed
    while (calls < maxcalls && !_connectors.empty() && _state != State::connecting)
    {
        // get the oldest connector
        auto *connector = _connectors.front();
        
        // remove it from the vector
        _connectors.pop_front();
        
        // report the connection (note that it is technically possible (but unlikely) that the socket is 
        // already in a lost state by now, but we still report it as 'connected', which has the consequence
        // that callers might use this socket to send out messages anyway, so they should check the return
        // value of the send() method)
        bool result = _state == State::failed ? connector->onFailure(_ip) : connector->onConnected(_ip, this);
        
        // if there was no call to user-space we can proceed (`this` cannot be destructed)
        if (!result) continue;
        
        // update bookkeeping
        calls += 1;
        
        // stop if userspace destructed us
        if (!watcher.valid()) return calls;
    }

    // call base to deliver buffered responses to the lookups
    calls += Socket::process(maxcalls - calls);

    // stop if userspace destructed us
    if (!watcher.valid()) return calls;

    // if the connection was lost while we have subscribers, we notify them as well
    while (maxcalls > calls && _state == State::lost && !_processors.empty())
    {
        // front of the set
        auto iter = _processors.begin();
        
        // get the oldest processor
        auto *processor = std::get<2>(*iter);
        
        // remove from the set
        _processors.erase(iter);
        
        // notify the processor
        if (!processor->onLost(_ip)) continue;
        
        // update bookkeeping
        calls += 1;
        
        // stop if userspace destructed us
        if (!watcher.valid()) return calls;
    }
    
    // check if the socket is unused now (our reset() method has some safety checks)
    reset();
    
    // done
    return calls;
}

/**
 *  Subscribe to this socket (wait for the socket to be connected)
 *  @param  connector   the object that subscribes
 *  @return Connecting  the object that can be used to unsubscribe
 */
Connecting *Tcp::subscribe(Connector *connector)
{
    // this is not possible if the connection is already in a failed state (in all other
    // cases (even state_lost!) subscribing is possible and will eventually result in a call 
    // to either onConnected() or onFailed())
    if (_state == State::failed) return nullptr;
    
    // add the connector to be notified later when the connection is available
    _connectors.push_back(connector);
    
    // in case we're not yet connected we already wait for the connection to be ready,
    // and when we already have connectors, the parent was already informed
    if (_state == State::connecting || _connectors.size() > 1) return this;
    
    // notify the parent that there are actions to be performed
    _handler->onActive(this);
    
    // report success
    return this;
}

/**
 *  Unsubscribe from the socket (in case a connector is no longer interested in the connect
 *  @param  connector   the object that unsubscribes
 */
void Tcp::unsubscribe(Connector *connector)
{
    // remove the connector from the vector
    _connectors.erase(std::remove(_connectors.begin(), _connectors.end(), connector), _connectors.end());
    
    // check if the socket is unused now (our reset() method has some safety checks)
    reset();
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
    
    // notify our parent (note that this will immediately destruct `this`)
    handler->onUnused(this);
}

/**
 *  End of namespace
 */
}

