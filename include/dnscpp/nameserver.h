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
#include <memory>

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
     *  The core
     *  @var Core*
     */
    Core *_core;

    /**
     *  IP address of the nameserver
     *  @var    Ip
     */
    Ip _ip;
    
    /**
     *  UDP sockets to use
     *  @var    std::vector<std::unique_ptr<Udp>>
     */
    std::vector<std::unique_ptr<Udp>> _udp;

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
    virtual void onReceived(const Ip &ip, const unsigned char *buffer, size_t size) override;


public:
    /**
     *  Constructor
     *  @param  core    the core object with the settings and event loop
     *  @param  ip      nameserver IP
     *  @throws std::runtime_error
     */
    Nameserver(Core *core, const Ip &ip) : _core(core), _ip(ip)
    {
        // we always want to have at least one udp socket ready
        _udp.emplace_back(new Udp(core, this));
    }
    
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
    bool datagram(const Query &query);

    /**
     *  Subscribe to the socket if you want to be notified about incoming responses
     *  @param  handler     the handler that wants to receive an answer
     *  @param  id          id of the response that the handler is interested in
     */
    void subscribe(Handler *handler, uint16_t id);
    
    /**
     *  Unsubscribe from the socket, this is the counterpart of subscribe()
     *  @param  handler     the handler that unsubscribes
     *  @param  id          id of the response that the handler is interested in
     */
    void unsubscribe(Handler *handler, uint16_t id);
};

/**
 *  End of namespace
 */
}
