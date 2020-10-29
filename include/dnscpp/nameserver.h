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
#include <unordered_map>

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
     *  Multimap with the handlers. We use an unordered_multimap instead of an unordered_set
     *  because we actually need to iterate over all the elements with the same id (there may be
     *  multiple elements with the same id, although in practice this doesn't happen often).
     *  @var set
     */
    std::unordered_multimap<uint16_t,Handler*> _handlers;

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
        
            // filter on the response id
            auto range = _handlers.equal_range(response.id());

            // iterate over those elements, notifying each handler
            for (auto iter = range.first; iter != range.second; ) 
            {
                // first we make a copy of the new iterator (since the onReceived might invalidate it)
                auto element = iter++;

                // call the onreceived for the element
                element->second->onReceived(this, response);
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
        _handlers.emplace(id, handler);
    }
    
    /**
     *  Unsubscribe from the socket, this is the counterpart of subscribe()
     *  @param  handler     the handler that unsubscribes
     *  @param  id          id of the response that the handler is interested in
     */
    void unsubscribe(Handler *handler, uint16_t id)
    {
        // find the iterator within a range
        auto range = _handlers.equal_range(id);

        // actually find the handler to erase
        auto iter = std::find_if(range.first, range.second, [handler](const decltype(_handlers)::value_type &other) { return other.second == handler; });

        // erase that element
        _handlers.erase(iter);

        // if nobody is listening to the socket, we can just as well close it
        if (_handlers.empty()) _udp.close();
    }
};

/**
 *  End of namespace
 */
}
