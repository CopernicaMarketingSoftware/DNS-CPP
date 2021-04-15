/**
 *  Inbound.h
 * 
 *  Base class for the UDP socket that contains just the functionality
 *  of remembering which processors are connected to a UDP socket.
 *  Or: which lookups are still interested in a response.
 * 
 *  We have this base-class/derived-class architecture to have a limited
 *  interface for the UDP socket: some classes inside DNS-CPP only need
 *  access to the unsubscribe mechanism, and not to the full socket.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <set>
#include <stdint.h>
#include <tuple>
#include "ip.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Processor;

/**
 *  Class definition
 */
class Inbound
{
protected:
    /**
     *  Set with the handlers (we originally used a multimap, but a std::set turned out to be more efficient)
     *  @var set
     */
    std::set<std::tuple<uint16_t,Ip,Processor*>> _processors;


    /**
     *  Constructor
     */
    Inbound() = default;
    
    /**
     *  No copying
     *  @param  that
     */
    Inbound(const Inbound &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Inbound() = default;
    
    /**
     *  Method that is called when the object is empty
     */
    virtual void close() = 0;


public:
    /**
     *  Subscribe for responses from a certain query-ID received from a certain IP
     *  @param  processor       the object that no longer is active
     *  @param  ip              the IP to which it was listening to
     *  @param  id              the query ID in which it was interested
     */
    void subscribe(Processor *processor, const Ip &ip, uint16_t id);

    /**
     *  Unsubscribe (counter-part of the subscribe method above)
     *  @param  processor       the object that no longer is active
     *  @param  ip              the IP to which it was listening to
     *  @param  id              the query ID in which it was interested
     */
    void unsubscribe(Processor *processor, const Ip &ip, uint16_t id);
};
    
/**
 *  End of namespace
 */
}

