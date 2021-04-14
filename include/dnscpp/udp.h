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
#include <list>
#include <string>
#include <set>

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
class Udp : private Monitor, private Watchable
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
     *  Set with the handlers (we originally used a multimap, but a std::set turned out to be more efficient)
     *  @var set
     */
    std::set<std::tuple<uint16_t,Ip,Processor*>> _processors;

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
     *  @param  processor   the object that is sending (and that will be notified of all future responses)
     *  @param  ip          IP address of the target nameserver
     *  @param  query       the query to send
     *  @param  buffersize
     *  @return Udp
     * 
     *  @todo   buffersize is strange
     */
    Udp *send(Processor *processor, const Ip &ip, const Query &query, int buffersize);

    /**
     *  Unsubscribe from the socket
     *  @param  processor   the object that is sending (and that will be notified of all future responses)
     *  @param  ip          IP address of the target nameserver
     *  @param  query       the query that was sent
     *  @return bool
     *
     *  @todo should be in special class
     */
    bool unsubscribe(Processor *processor, const Ip &ip, const Query &query);

    /**
     *  Close the socket (this is useful if you do not expect incoming data anymore)
     *  The socket will be automatically opened if you start sending to it
     *  @return bool
     * 
     *  @todo should / could be private?
     */
    bool close();

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
