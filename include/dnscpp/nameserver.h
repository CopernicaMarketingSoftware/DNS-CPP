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
#include <map>

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
class Nameserver : private Udp::Handler, private Watchable
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
    Udp _udp;

    /**
     *  All the buffered responses that came in 
     *  @var std::list
     */
    std::list<Received> _responses;

    /**
     *  Set with the handlers
     *  @var set
     */
    std::map<uint16_t,Handler*> _handlers;

    /**
     *  Method that is called when a response is received
     *  @param  now         the receive-time
     *  @param  address     the address of the nameserver from which it is received
     *  @param  buffer      the received response
     *  @param  size        size of the response
     */
    virtual void onReceived(time_t now, const struct sockaddr *address, const unsigned char *buffer, size_t size) override;


public:
    /**
     *  Constructor
     *  @param  core    the core object with the settings and event loop
     *  @param  ip      nameserver IP
     *  @throws std::runtime_error
     */
    Nameserver(Core *core, const Ip &ip);
    
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
        _handlers[id] = handler;
    }
    
    /**
     *  Unsubscribe from the socket, this is the counterpart of subscribe()
     *  @param  id          id of the response that the handler is interested in
     */
    void unsubscribe(uint16_t id)
    {
        // simply erase the element
        _handlers.erase(id);

        // if nobody is listening to the socket any more, we can just as well close it
        if (_handlers.empty()) _udp.close();
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
