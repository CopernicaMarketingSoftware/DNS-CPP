/**
 *  Nameserver.h
 * 
 *  Class that encapsulates everything we know about one nameserver,
 *  and the socket that we use to communicate with that nameserver.
 * 
 *  This is an internal class. You normally do not have to construct
 *  nameserver instances yourself, as you can send out your queries
 *  to multiple nameservers in parallel via the Conext class.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @author Michael van der Werve <michael.vanderwerve@mailerq.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "udp.h"
#include "ip.h"
#include "response.h"
#include "timer.h"
#include "watchable.h"
#include <set>

/**
 *  Begin of the namespace
 */
namespace DNS {

/**
 *  Forward declaration
 */
class Core;

/**
 *  Class definition
 */
class Nameserver : private Watchable
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
         *  @param  nameserver  the reporting nameserver
         *  @param  response    the received response
         *  @return bool        was the response processed?
         */
        virtual bool onReceived(Nameserver *nameserver, const Response &response) = 0;
    };
    
private:
    /**
     *  Pointer to the core object
     *  @var    Core
     */
    Core *_core;

    /**
     *  IP address of the nameserver
     *  @var    Ip
     */
    Ip _ip;
    
    /**
     *  UDP socket to send messages to the nameserver
     *  @var    Udp
     */
    Udp *_udp;

    /**
     *  All the buffered responses that came in 
     *  @var std::list
     */
    std::list<std::basic_string<unsigned char>> _responses;

    /**
     *  Set with the handlers (we use this as a multimap, but a std::set is more efficient)
     *  @var set
     */
    std::set<std::pair<uint16_t,Handler*>> _handlers;
public:
    /**
     *  Constructor
     *  @param  core    the core object with the settings and event loop
     *  @param  ip      nameserver IP
     *  @param  udp     udp
     *  @throws std::runtime_error
     */
    Nameserver(Core *core, const Ip &ip, Udp *udp);
    
    /**
     *  No copying
     *  @param  that    other nameserver
     */
    Nameserver(const Nameserver &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Nameserver();
    
    /**
     *  Expose the nameserver IP
     *  @return Ip
     */
    const Ip &ip() const { return _ip; }

    /**
     *  Method that is called when a response is received
     *  @param  buffer      the received response
     *  @param  size        size of the response
     */
    void receive(const unsigned char *buffer, size_t size);
    
    /**
     *  Send a datagram to the nameserver
     *  @param  query
     *  @return bool
     */
    bool datagram(const Query &query);

    /**
     *  Subscribe to the socket if you want to be notified about incoming responses
     *  @param  handler     the handler that wants to receive an answer
     *  @param  id          id of the response that the handler is interested in
     */
    void subscribe(Handler *handler, uint16_t id)
    {
        // emplace the handler
        _handlers.insert(std::make_pair(id, handler));
    }
    
    /**
     *  Unsubscribe from the socket, this is the counterpart of subscribe()
     *  @param  handler     the handler that unsubscribes
     *  @param  id          id of the response that the handler is interested in
     */
    void unsubscribe(Handler *handler, uint16_t id)
    {
        // simply erase the element
        _handlers.erase(std::make_pair(id, handler));
    }
    
    /**
     *  Is the nameserver busy (meaning: is there a backlog of unprocessed messages?)
     *  @return bool
     */
    bool busy() const { return !_responses.empty(); }

    /**
     *  Process cached responses (this is an internal method)
     *  @param  maxcalls    max number of calls to userspace
     *  @return size_t      number of processed answers
     *  @internal
     */
    size_t process(size_t maxcalls);

};

/**
 *  End of namespace
 */
}
