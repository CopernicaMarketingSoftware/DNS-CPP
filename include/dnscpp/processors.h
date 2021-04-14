/**
 *  Processors.h
 * 
 *  Base class for the UDP socket that contains just the functionality
 *  of remembering which processors are connected to a UDP socket.
 *  Or: which lookups are still interested in a response.
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
 * 
 *  @todo this is a stupid name, because the public interface is only used to unsubscribe, maybe Listener is a better name?
 */
class Processors
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
    Processors() = default;
    
    /**
     *  Destructor
     */
    virtual ~Processors() = default;
    
    /**
     *  Method that is called when the object is empty
     */
    virtual void close() = 0;

    /**
     *  Add a processor
     *  @param  processor       the object that no longer is active
     *  @param  ip              the IP to which it was listening to
     *  @param  id              the query ID in which it was interested
     */
    void subscribe(Processor *processor, const Ip &ip, uint16_t id);

public:
    /**
     *  Remove a processor from the set
     *  @param  processor       the object that no longer is active
     *  @param  ip              the IP to which it was listening to
     *  @param  id              the query ID in which it was interested
     */
    void remove(Processor *processor, const Ip &ip, uint16_t id);
};
    
/**
 *  End of namespace
 */
}

