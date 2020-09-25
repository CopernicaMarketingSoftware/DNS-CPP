/**
 *  Udp.cpp
 * 
 *  Implementation file for the Udp class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/udp.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/query.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        event loop
 *  @param  version     ip version
 *  @param  handler     object that is notified about incoming messages
 *  @throws std::runtime_error
 */
Udp::Udp(Loop *loop, int version, Handler *handler) : 
    _loop(loop), 
    _fd(socket(version == 6 ? AF_INET6 : AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
    _handler(handler)
{
    // check for success
    if (_fd < 0) throw std::runtime_error("failed to open udp socket");
    
    // we want to be notified when the socket receives data
    _identifier = _loop->add(_fd, 1, this);
}

/**
 *  Destructor
 */
Udp::~Udp()
{
    // tell the event loop that we no longer are interested in notifications
    _loop->remove(_identifier, _fd, this);
    
    // close the socket
    close(_fd);
}

/**
 *  Method that is called from user-space when the socket becomes
 *  readable.
 */
void Udp::notify()
{
    // prevent exceptions (parsing the ip or the response could fail)
    try
    {
        // the buffer to receive the response in
        // @todo use a macro
        unsigned char buffer[65536];

        // structure will hold the source address (we use an ipv6 struct because that is also big enough for ipv4)
        struct sockaddr_in6 from; socklen_t fromlen = sizeof(from);
        
        // reveive the message
        auto bytes = recvfrom(_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
        
        // find the ip address from which the message was received
        Ip ip((struct sockaddr *)&from);
        
        // notify the handler
        _handler->onReceived(ip, Response(buffer, bytes));
    }
    catch (const std::runtime_error &error)
    {
        // report the error
    }
}

/**
 *  Send a query to a nameserver
 *  @param  ip      IP address of the nameserver
 *  @param  query   the query to send
 *  @return bool
 */
bool Udp::send(const Ip &ip, const Query &query)
{
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
        return send((struct sockaddr *)&info, sizeof(struct sockaddr_in6), query);
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
        return send((const sockaddr *)&info, sizeof(struct sockaddr_in), query);
    }
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
    return sendto(_fd, query.data(), query.size(), MSG_NOSIGNAL, address, size);
}

/**
 *  End of namespace
 */
}
