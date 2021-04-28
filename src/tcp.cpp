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
#include "../include/dnscpp/query.h"
#include "blocking.h"
#include "connector.h"
#include <cassert>

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
    // check if this query id is inflight
    if (_queryids.count(query.id()))
    {
        // it's already inflight. we'll put it in a queue to be sent later
        auto iter = _awaiting.find(query.id());

        // if there's no list of queries to be sent at this position
        if (iter == _awaiting.end())
        {
            // create the list now
            decltype(_awaiting)::mapped_type list;

            // put the first element into it
            list.push_back(query);

            // update the map
            _awaiting.emplace(query.id(), move(list));
        }
        else
        {
            // there's already a list at this key, so we can add another query to it
            iter->second.push_back(query);
        }

        // We'll pretend everything went OK. If it later turns out we couldn't
        // send it after all, we'll enter the `fail()` method.
        return this;
    }
    else
    {
        // this query is not inflight, let's remember that it is in fact in flight
        _queryids.insert(query.id());
    }

    // send it over the wire
    return sendimpl(query);
}

/**
 *  Blocking send this query
 *  @param  query  The query
 *  @return this or nullptr if something went wrong
 */
Inbound *Tcp::sendimpl(const Query &query)
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
    _connected = error() == 0;
    
    // if the connection failed
    if (!_connected) return fail();

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
    _queryids.clear();
    _awaiting.clear();

    // we connectors should be notified about the failure, but we postpone that so that
    // it can be triggered from the Core class via call to deliver() (so that Core can keep
    // all its timers up-to-date)
    _handler->onBuffered(this);
}

/**
 *  Check return value of a recv syscall
 *  @param  bytes  The bytes transferred
 *  @return true if we should leap out (an error occurred), false if not
 */
bool Tcp::updatetransferred(ssize_t result)
{
    // the operation would block, but don't leap out
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;

    // if there is a failure we leap out as well
    if (result <= 0) return fail(), true;

    // update the number of transferred bytes
    _transferred += result;

    // don't leap out
    return false;
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
    if (!_connected) return upgrade();

    // We can be in two receive states: the first state is that we're waiting for the
    // size of the buffer. The second state is that we are waiting for the response content itself.
    // To determine in what state we're in, we can check how many bytes have been transferred.
    // If that's less than 2 then we're still waiting for the response size.
    if (_transferred < sizeof(uint16_t))
    {
        const auto result = ::recv(_fd, (uint8_t*)&_size + _transferred, sizeof(uint16_t) - _transferred, MSG_DONTWAIT);

        // if there is a failure we leap out
        if (updatetransferred(result)) return;

        // if we still haven't received the two bytes we should leap out here
        else if (_transferred < sizeof(uint16_t)) return;

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
    if (updatetransferred(::recv(_fd, _buffer.data() + offset, _buffer.size() - offset, MSG_DONTWAIT))) return;

    // continue waiting if we have not yet received everything there is
    if (expected() > 0) return;

    // all data has been received, we can move the response content into a deferred list to be processed later
    add(_ip, move(_buffer));
    
    // for the next response we empty the buffer again
    _transferred = 0;
}

/**
 *  A response payload was received with this ID
 *  @param  id    The identifier
 */
void Tcp::onReceivedId(uint16_t id)
{
    // leap out if we're in the failure state
    if (!_connected && _identifier == nullptr) return;

    // find the list of queries that are awaiting to be sent
    auto iter = _awaiting.find(id);

    // if there are no queries with this ID, we can leap out now
    if (iter == _awaiting.end())
    {
        // forget that this query is in flight
        _queryids.erase(id);

        // leap out
        return;
    }

    // get the list of queries to be sent with this ID
    auto &list = iter->second;

    // send it now
    if (sendimpl(list.front()))
    {
        // remove it from the list
        list.pop_front();

        // if the list is now empty, we can remove it from the `_awaiting` map
        if (list.empty()) _awaiting.erase(iter);
    }
    else
    {
        // oops, forget about this tcp connection
        fail();
    }
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
 *  @return Connecting  the object that can be used to unsubscribe
 */
Connecting *Tcp::subscribe(Connector *connector)
{
    // this is not possible if the connection is already in a failed state
    if (!_connected && _identifier == nullptr) return nullptr;
    
    // add the connector to be notified later when the connection is available
    _connectors.push_back(connector);
    
    // in case we're not yet connected we already wait for the connection to be ready,
    // and when we already have connectors, the parent was already informed
    if (!_connected || _connectors.size() > 1) return this;
    
    // notify the parent that there are actions to be performed
    _handler->onBuffered(this);
    
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
    
    // if the socket is unused by now, we have to inform our parent
    if (!_connectors.empty() || subscribers() > 0) return;
    
    // there are no more subscribers, we are going to tell the parent about it
    auto *handler = (Tcp::Handler *)_handler;
    
    // notify our parent (note that this will immediately destruct `this`)
    handler->onUnused(this);
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

