/**
 *  Udp.h
 *
 *  Internal class that implements a UDP socket over which messages
 *  can be sent to nameservers. You normally do not have to construct
 *  this class in user space, it is used internally by the Context class.
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
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
#include <list>
#include <string>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Core;
class Query;
class Loop;
class Ip;
class Response;

/**
 *  Class definition
 */
class Udp
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
         *  @param  time        receive-time
         *  @param  address     the address of the nameserver from which it is received
         *  @param  response    the received response
         *  @param  size        size of the response
         */
        virtual void onReceived(time_t now, const struct sockaddr *addr, const unsigned char *response, size_t size) = 0;
    };
    
private:
    /**
     *  event loop
     *  @var Loop*
     */
    Loop *_loop;

    struct Socket final : private Monitor
    {
        /**
         *  Pointer to a Udp object
         *  @var Udp*
         */
        Udp *parent = nullptr;

        /**
         *  User space identifier of this monitor
         *  @var void *
         */
        void *identifier = nullptr;

        /**
         *  The filedescriptor of the socket
         *  @var int
         */
        int fd = -1;

        Socket(Udp *parent);
        ~Socket();

        bool valid() const noexcept { return fd > 0; }
        void notify() override;
        bool send(const Ip &ip, const Query &query);
        bool send(const struct sockaddr *address, size_t size, const Query &query);
        bool open(int version, int buffersize);
        bool close();

        /**
         *  Helper method to set an integer socket option
         *  @param  optname
         *  @param  optval
         */
        int setintopt(int optname, int32_t optval);
    };

    friend class Socket;

    /**
     *  The object that is interested in handling responses
     *  @var Handler*
     */
    Handler *_handler;

    std::vector<Socket> _sockets;
    size_t _current = 0;

    /**
     *  Size of the send and receive buffer. If set to zero, default
     *  will be kept. This is limited by the system maximum (wmem_max and rmem_max)
     *  @var size_t
     */
    int32_t _buffersize = 0;

public:
    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  handler     object that will receive all incoming responses
     *  @param  socketcount number of UDP sockets to keep open
     *  @param  buffersize  send & receive buffer size of each UDP socket
     *  @throws std::runtime_error
     */
    Udp(Loop *loop, Handler *handler, size_t socketcount = 1, int buffersize = 0);

    /**
     *  No copying
     *  @param  that
     */
    Udp(const Udp &that) = delete;
        
    /**
     *  Destructor
     */
    virtual ~Udp() = default;

    /**
     *  Send a query to the socket
     *  Watch out: you need to be consistent in calling this with either ipv4 or ipv6 addresses
     *  @param  ip      IP address of the target nameserver
     *  @param  query   the query to send
     *  @return bool
     */
    bool send(const Ip &ip, const Query &query);

    void close();
};
    
/**
 *  End of namespace
 */
}
