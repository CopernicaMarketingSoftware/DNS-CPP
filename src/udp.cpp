/**
 *  Udp.cpp
 * 
 *  Implementation file for the Udp class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/udp.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/now.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>
#include <poll.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        event loop
 *  @throws std::runtime_error
 */
Udp::Udp(Loop *loop) : _loop(loop) {}

/**
 *  Destructor
 */
Udp::~Udp()
{
    // close the socket
    close();
}

/**
 *  Helper method to set a socket option
 *  @param  optname
 *  @param  optval
 *  @param  optlen
 */
int Udp::setintopt(int optname, int32_t optval)
{
    // set the socket option
    return setsockopt(_fd, SOL_SOCKET, optname, &optval, 4);
}

/**
 *  Open the socket
 *  @param  version
 *  @param  buffersize
 *  @return bool
 */
bool Udp::open(int version, int buffersize)
{
    // if already open
    if (_fd >= 0) return true;
    
    // try to open it (note that we do not set the NONBLOCK option, because we have not implemented 
    // buffering for the sendto() call (this could be a future optimization)
    _fd = socket(version == 6 ? AF_INET6 : AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    
    // check for success
    if (_fd < 0) return false;

    // if there is a buffer size to set, do so
    if (buffersize > 0)
    {
        // set the send and receive buffer to the requested buffer size
        setintopt(SO_SNDBUF, buffersize);
        setintopt(SO_RCVBUF, buffersize);
    }

    // we want to be notified when the socket receives data
    _identifier = _loop->add(_fd, 1, this);
    
    // done
    return true;
}

/**
 *  Close the socket
 *  @return bool
 */
bool Udp::close()
{
    // if already closed
    if (_fd < 0) return false;

    // tell the event loop that we no longer are interested in notifications
    _loop->remove(_identifier, _fd, this);
    
    // close the socket
    ::close(_fd);
    
    // remember that socket is closed
    _fd = -1; _identifier = nullptr;
    
    // done
    return true;
}

/**
 *  Is the socket now readable?
 *  @return bool
 */
bool Udp::readable() const
{
    // if not active
    if (_fd < 0) return false;
    
    // structure required by the poll() call
    pollfd info;
    
    // fill the structure
    info.fd = _fd;
    info.events = POLLIN;
    info.revents = 0;
    
    // do the call
    return poll(&info, 1, 0) > 0;
}

/**
 *  Method that is called from user-space when the socket becomes readable.
 *  @param  now
 */
void Udp::notify()
{
    // do nothing if there is no socket (how is that possible!?)
    if (_fd < 0) return;
    
    // the buffer to receive the response in
    // @todo use a macro
    unsigned char buffer[65536];

    // structure will hold the source address (we use an ipv6 struct because that is also big enough for ipv4)
    struct sockaddr_in6 from; socklen_t fromlen = sizeof(from);

    // get current time
    Now now;

    // we want to get as much messages at onces as possible, but not run forever
    // @todo use scatter-gather io to optimize this further
    for (size_t messages = 0; messages < 1024; ++messages)
    {
        // reveive the message (the DONTWAIT option is needed because this is a blocking socket, but we dont want to block now)
        auto bytes = recvfrom(_fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&from, &fromlen);
        
        // if there were no bytes, leap out
        if (bytes <= 0) break;

        // pass to the handler
        // @todo lookup the right processor
        //_processor->onReceived(now, (struct sockaddr *)&from, buffer, bytes);
    }
    
    // @todo reschedule the processing of messages
    //reschedule(now);
}

/**
 *  Remember that a certain response was received (so that we can process it later,
 *  when we have time for that, we now want to buffer the incoming messages fast)
 *  @param  addr        the nameserver from which this message came
 *  @param  response    response buffer
 *  @param  size        buffer size
 */
void Udp::remember(const struct sockaddr *addr, const unsigned char *response, size_t size)
{
    // avoid exceptions (in case the ip cannot be parsed)
    try
    {
        // remember the response for now
        // @todo make this more efficient
        _responses.emplace_back(std::make_pair(addr, std::basic_string<unsigned char>(response, size)));
    }
    catch (...)
    {
        // ip address could not be parsed
    }
}

/**
 *  Send a query to a nameserver (+open the socket when needed)
 *  @param  processor   the object that will be notified of responses
 *  @param  ip          IP address of the nameserver
 *  @param  query       the query to send
 *  @param  buffersize
 *  @return Udp         the object from which the user can unsubscribe
 * 
 *  @todo   return a different type of object
 */
Udp *Udp::send(Processor *processor, const Ip &ip, const Query &query, int buffersize)
{
    // if the socket is not yet open we need to open it
    if (_fd < 0 && !open(ip.version(), buffersize)) return nullptr;

    // should we bind in the ipv4 or ipv6 fashion?
    if (ip.version() == 6)
    {
        // structure to initialize
        struct sockaddr_in6 info;

        // fill the members
        info.sin6_family = AF_INET6;
        info.sin6_port = htons(53);
        info.sin6_flowinfo = 0;
        info.sin6_scope_id = 0;

        // copy the address
        memcpy(&info.sin6_addr, (const struct in6_addr *)ip, sizeof(struct in6_addr));
        
        // pass on to other method
        if (!send((struct sockaddr *)&info, sizeof(struct sockaddr_in6), query)) return nullptr;
    }
    else
    {
        // structure to initialize
        struct sockaddr_in info;

        // fill the members
        info.sin_family = AF_INET;
        info.sin_port = htons(53);

        // copy address
        memcpy(&info.sin_addr, (const struct in_addr *)ip, sizeof(struct in_addr));

        // pass on to other method
        if (!send((const sockaddr *)&info, sizeof(struct sockaddr_in), query)) return nullptr;
    }
    
    // subscribe this processor, it will be notified when we receive a response for this query
    _processors.emplace(query.id(), ip, processor);
    
    // expose the object with subscriptions
    return this;
}

/**
 *  Send a query to a certain nameserver
 *  @param  address     target address
 *  @param  size        size of the address
 *  @param  query       query to send
 *  @return bool
 */
bool Udp::send(const struct sockaddr *address, size_t size, const Query &query)
{
    // send over the socket
    // @todo include MSG_DONTWAIT + implement non-blocking????
    return sendto(_fd, query.data(), query.size(), MSG_NOSIGNAL, address, size) >= 0;
}

/**
 *  End of namespace
 */
}
