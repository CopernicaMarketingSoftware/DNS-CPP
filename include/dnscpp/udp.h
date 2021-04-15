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
#include "watchable.h"
#include "inbound.h"
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
class Processor;

/**
 *  Class definition
 */
class Udp : private Monitor, private Watchable, private Inbound
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Method that is called when the udp socket has a buffer available
         *  @param  udp     the reporting socket
         */
        virtual void onBuffered(Udp *udp) = 0;
    };

private:
    /**
     *  Pointer to our parent object
     *  @var Handler
     */
    Handler *_handler;

    /**
     *  event loop
     *  @var Loop*
     */
    Loop *_loop;
    
    /**
     *  The filedescriptor of the socket
     *  @var int
     */
    int _fd = -1;
    
    /**
     *  User space identifier of this monitor
     *  @var void *
     */
    void *_identifier = nullptr;

    /**
     *  All the buffered responses that came in 
     *  @var std::list
     */
    std::list<std::pair<Ip,std::basic_string<unsigned char>>> _responses;

    /**
     *  Helper method to set an integer socket option
     *  @param  optname
     *  @param  optval
     */
    int setintopt(int optname, int32_t optval);

    /**
     *  Method that is called from user-space when the socket becomes readable.
     */
    virtual void notify() override;
    
    /**
     *  Send a query to a certain nameserver
     *  @param  address     target address
     *  @param  size        size of the address
     *  @param  query       query to send
     *  @return bool
     */
    bool send(const struct sockaddr *address, size_t size, const Query &query);

    /**
     *  Open the socket (this is optional, the socket is automatically opened when you start sending to it)
     *  @param  version
     *  @param  buffersize
     *  @return bool
     */
    bool open(int version, int buffersize);

    /**
     *  Close the socket (this is useful if you do not expect incoming data anymore)
     *  The socket will be automatically opened if you start sending to it
     */
    virtual void close() override;

    /**
     *  Remember that a certain response was received (so that we can process it later,
     *  when we have time for that, we now want to buffer the incoming messages fast)
     *  @param  addr        the nameserver from which this message came
     *  @param  response    response buffer
     *  @param  size        buffer size
     */
    void schedule(const struct sockaddr *addr, const unsigned char *response, size_t size);


public:
    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  handler     parent object
     *  @throws std::runtime_error
     */
    Udp(Loop *loop, Handler *handler);
    
    /**
     *  No copying
     *  @param  that
     */
    Udp(const Udp &that) = delete;
        
    /**
     *  Destructor
     */
    virtual ~Udp();

    /**
     *  Send a query to the socket
     *  Watch out: you need to be consistent in calling this with either ipv4 or ipv6 addresses
     *  @param  ip          IP address of the target nameserver
     *  @param  query       the query to send
     *  @param  buffersize  strange parameter
     *  @return Inbound     the inbound object over which the message is sent
     * 
     *  @todo   buffersize is strange
     */
    Inbound *send(const Ip &ip, const Query &query, int buffersize);

    /**
     *  Deliver messages that have already been received and buffered to their appropriate processor
     *  @param  size_t      max number of calls to userspace
     *  @return size_t      number of processed answers
     */
    size_t deliver(size_t maxcalls);

    /**
     *  Is the socket now readable?
     *  @return bool
     */
    bool readable() const;
    
    /**
     *  Does the socket have an inbound buffer (meaning: is there a backlog of unprocessed messages?)
     *  @return bool
     */
    bool buffered() const { return !_responses.empty(); }
};
    
/**
 *  End of namespace
 */
}
