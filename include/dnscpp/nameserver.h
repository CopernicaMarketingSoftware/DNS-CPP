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
         *  @return bool        was the response processed?
         */
        virtual bool onReceived(Nameserver *nameserver, const Response &response) = 0;
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

            // iterate over those elements, notifying each handler
            for (auto iter = begin; iter != _handlers.end(); ++iter) 
            {
                // if this element is not applicable any more, we're going to leap out (we're done)
                if (iter->first != response.id()) return;

                // call the onreceived for the element, if it returns true, it has acknowledged that it
                // processed the response and so we can stop iterating. we have to, since it might have removed
                // itself so iter may be broken at this point.
                if (iter->second->onReceived(this, response)) return;
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
     *  @param  core    the core object with the settings and event loop
     *  @param  ip      nameserver IP
     *  @throws std::runtime_error
     */
    Nameserver(Core *core, const Ip &ip) : _ip(ip), _udp(core, this) {}
    
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

        // simply erase the element
        _handlers.erase(iter);

        // if nobody is listening to the socket any more, we can just as well close it
        if (_handlers.empty()) _udp.close();
    }
    
    /**
     *  Is the nameserver readable / do we already have received some data?
     *  @return bool
     */
    bool readable() const
    {
        // pass on
        return _udp.readable();
    }
    
    /**
     *  The oldest and newest buffered, unprocssed, message, when was it received?
     *  @return time_t
     */
    time_t oldest() const { return _udp.oldest(); }
    time_t newest() const { return _udp.newest(); }
};

/**
 *  End of namespace
 */
}
