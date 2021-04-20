#include "../include/dnscpp/udp.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/watcher.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/processor.h"
#include "../include/dnscpp/query.h"
#include <unistd.h>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Constructor does nothing but store a pointer to a Udp object.
 *  Sockets are opened lazily
 *  @param  loop        the event loop
 *  @param  handler     object that is notified in case of events
 */
Udp::Udp(Loop *loop, Socket::Handler *handler) : Socket(handler), _loop(loop) {}

/**
 *  Closes the file descriptor
 */
Udp::~Udp() { close(); }

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
 *  @return bool
 */
bool Udp::open(int version)
{
    // if already open
    if (_fd >= 0) return true;

    // try to open it (note that we do not set the NONBLOCK option, because we have not implemented
    // buffering for the sendto() call (this could be a future optimization)
    _fd = socket(version == 6 ? AF_INET6 : AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);

    // check for success
    if (_fd < 0) return false;

    // we want to be notified when the socket receives data
    _identifier = _loop->add(_fd, 1, this);

    // if there is a buffer size to set, do so
    if (_buffersize == 0) return true;

    // set the send and receive buffer to the requested buffer size
    setintopt(SO_SNDBUF, _buffersize);
    setintopt(SO_RCVBUF, _buffersize);

    // done
    return true;
}

/**
 *  Close the socket
 */
void Udp::close()
{
    // if already closed
    if (!valid()) return;

    // tell the event loop that we are no longer are interested in notifications
    _loop->remove(_identifier, _fd, this);

    // close the socket
    ::close(_fd);

    // remember that socket is closed
    _fd = -1; _identifier = nullptr;
}

/**
 *  Method that is called when there are no more subscribers, and that 
 *  is implemented in the derived classes. Watch out: this method can be called
 *  in the middle of loop through sockets so the implementation must be careful.
 */
void Udp::reset()
{
    // we simply close the socket, it will be reopenen when we need it for the next datagram
    close();
}

/**
 *  Send a query over this socket
 *  @param  ip          IP address of the nameserver
 *  @param  query       the query to send
 *  @return Inbound     the object that will receive the inbound response
 */
Inbound *Udp::send(const Ip &ip, const Query &query)
{
    // if the socket is not yet open we need to open it
    if (!open(ip.version())) return nullptr;

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

    // everything went OK!
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
 *  Method that is called from user-space when the socket becomes readable.
 *  @param  now
 */
void Udp::notify()
{
    // do nothing if there is no socket (how is that possible!?)
    if (!valid()) return;

    // the buffer to receive the response in
    // @todo use a macro
    unsigned char buffer[65536];

    // structure will hold the source address (we use an ipv6 struct because that is also big enough for ipv4)
    struct sockaddr_in6 from; socklen_t fromlen = sizeof(from);

    // we want to get as much messages at onces as possible, but not run forever
    // @todo use scatter-gather io to optimize this further
    for (size_t messages = 0; messages < 1024; ++messages)
    {
        // reveive the message (the DONTWAIT option is needed because this is a blocking socket, but we dont want to block now)
        auto bytes = recvfrom(_fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&from, &fromlen);

        // if there were no bytes, leap out
        if (bytes <= 0) break;
        
        // add to the buffer
        add((const sockaddr *)&from, buffer, bytes);
    }
}


/**
 *  End namespace
 */
}
