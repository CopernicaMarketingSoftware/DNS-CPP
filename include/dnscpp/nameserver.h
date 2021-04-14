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
class Nameserver
{
private:
    /**
     *  Pointer to the core object
     *  @var    Core
     * 
     *  @todo do we still need this?
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

public:
    /**
     *  Constructor
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
     *  Send a datagram to the nameserver
     *  @param  processor   the sending object that will be notified of all future responses
     *  @param  query       the query to send
     *  @return Udp*        the socket over which the request was sent
     * 
     *  @todo use a different return-type to not expose the entire Udp class
     */
    Udp *datagram(Processor *processor, const Query &query);
};

/**
 *  End of namespace
 */
}
