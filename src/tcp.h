/**
 *  Tcp.h
 * 
 *  Class that takes care of setting up a tcp connection and that waits
 *  for a response
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
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
#include "blocking.h"
#include "../include/dnscpp/loop.h"

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Class definition
 */
class Tcp
{
private:
    /**
     *  The event loop to which the filedescriptor is bound
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  The filedescriptor
     *  @var int
     */
    int _fd;
    
    
public:
    /**
     *  Constructor
     *  @param  loop        user space event loop
     *  @throws std::runtime_error
     */
    Tcp(Loop *loop, const Ip &ip) : 
        _loop(loop),
        _fd(socket(ip.version() == 6 ? AF_INET6 : AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
    {
        // check if the socket was correctly created
        if (_fd < 0) throw std::runtime_error("failed to create socket");

        // we are going to set the nodelay flag to enforce that all data is immediately sent
        int nodelay = true;

        // set the option
        setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
    }
    
    /**
     *  Destructor
     */
    virtual ~Tcp()
    {
        // close the socket
        close(_fd);
    }

    /**
     *  Helper function to make a connection
     *  @param  address     the address to connect to
     *  @param  size        size of the address structure
     *  @return bool
     */
    bool connect(struct sockaddr *address, size_t size)
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
    bool connect(const Ip &ip, uint16_t port)
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
     *  The error state of the socket -- this can be used to check whether the socket is connected
     *  @return int
     */
    int error() const
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
     *  @return bool        was the query sent?
     */
    ssize_t send(const Query &query)
    {
        // make the socket blocking
        Blocking blocking(_fd);
        
        // the first two bytes should contain the message size
        uint16_t size = htons(query.size());
        
        // send the size
        auto result1 = ::send(_fd, &size, sizeof(size), MSG_NOSIGNAL | MSG_MORE);
        
        // was this a success?
        if (result1 < 2) return false;
        
        // now send the entire query
        auto result2 = ::send(_fd, query.data(), query.size(), MSG_NOSIGNAL);
        
        // report the result
        return result2 >= (ssize_t)query.size();
    }
    
    /**
     *  Receive data from the connection
     *  @param  buffer      buffer to be filled
     *  @param  size        size of the buffer
     *  @return ssize_t     number of bytes received
     */
    ssize_t receive(unsigned char *buffer, size_t size)
    {
        // pass on
        return ::recv(_fd, buffer, size, 0);
    }
    
    /**
     *  Monitor the socket for certain activity
     *  @param  events      the events to monitor for (readability and/or writability)
     *  @param  monitor     the monitor to attach
     *  @return void*       identifier of the monitor (you need this to remove the monitor)
     */
    void *monitor(int events, Monitor *monitor)
    {
        // pass on to the loop
        return _loop->add(_fd, events, monitor);
    }
    
    /**
     *  Remove a monitor from the socket
     *  @param  identifier  the identifier of the monitor
     *  @param  monitor     the original monitor object
     */
    void remove(void *identifier, Monitor *monitor)
    {
        // pass on to the loop
        _loop->remove(identifier, _fd, monitor);
    }
};
    
/**
 *  End of namespace
 */
}

