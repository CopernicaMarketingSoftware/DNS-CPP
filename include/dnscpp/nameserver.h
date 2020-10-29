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
 *  @copyright 2020 Copernica BV
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
#include <set>

/**
 *  Begin of the namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Nameserver : private Udp::Handler
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
         */
        virtual void onReceived(Nameserver *nameserver, const Response &response) = 0;
    };
    
    
private:
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
     *  Set with the handlers
     *  @var set
     */
    std::set<std::pair<uint16_t,Handler*>> _handlers;

    /**
     *  The next iterator we're going to use in onReceived.
     *  @var _handlers::const_iterator
     */
    decltype(_handlers)::const_iterator _iter;

    /**
     *  Method that is called when a response is received
     *  @param  ip          the ip of the nameserver from which it is received
     *  @param  buffer      the received response
     *  @param  size        size of the response
     */
    virtual void onReceived(const Ip &ip, const unsigned char *buffer, size_t size) override
    {
        // avoid exceptions (parsing the response could fail)
        try
        {
            // ignore responses from other ips
            // @todo also ignore messages that do not come from port 53???
            if (ip != _ip) return;
            
            // if nobody is interested there is no point in parsing the message
            if (_handlers.empty()) return;
            
            // parse the response
            Response response(buffer, size);
        
            // filter on the response, the beginning is simply the handler at nullptr
            auto begin = _handlers.lower_bound(std::make_pair(response.id(), nullptr));

            // we store the next iterator we're going to use. normally, we wouldn't do this, but since this
            // is the 'core' of the actual resolution, we do not want to make copies of the handlers and we 
            // need to be aware of iterator invalidations. this way, we can simply take the next element once
            // we invalidate it, not breaking the loop. if we do not do this and only invalidate the 'current'
            // element (or store the next one), we will (potentially) crash once another job is cancelled. 
            _iter = begin;

            // iterate over those elements, notifying each handler
            for (auto iter = _iter; iter != _handlers.end(); iter = _iter) 
            {
                // if this element is not applicable any more, we're going to leap out (we're done)
                if (iter->first != response.id()) return;

                // store the iterator we're going to work on
                _iter = std::next(iter);

                // call the onreceived for the element
                iter->second->onReceived(this, response);
            }
        }
        catch (...)
        {
            // parsing the response failed
        }
    }


public:
    /**
     *  Constructor
     *  @param  loop    event loop
     *  @param  ip      nameserver IP
     *  @throws std::runtime_error
     */
    Nameserver(Loop *loop, const Ip &ip) : _ip(ip), _udp(loop, this) {}
    
    /**
     *  No copying
     *  @param  that    other nameserver
     */
    Nameserver(const Nameserver &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Nameserver() = default;
    
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
    bool datagram(const Query &query)
    {
        // send the package
        return _udp.send(_ip, query);
    }

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
        // find the element
        auto iter = _handlers.find(std::make_pair(id, handler));

        // if it is not found, we leap out (this should not happen)
        if (iter == _handlers.end()) return;

        // if we happen to erase the element we're working on, the iterator will be invalidated so 
        // we need to move that iterator
        if (iter == _iter) _iter = _handlers.erase(iter);

        // otherwise we can simply erase it
        else _handlers.erase(iter);

        // if nobody is listening to the socket any more, we can just as well close it
        if (_handlers.empty()) _udp.close();
    }
};

/**
 *  End of namespace
 */
}
