/**
 *  Inbound.cpp
 * 
 *  Implementation file for the Inbound class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/inbound.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Add a processor
 *  @param  processor       the object that no longer is active
 *  @param  ip              the IP to which it was listening to
 *  @param  id              the query ID in which it was interested
 */
void Inbound::subscribe(Processor *processor, const Ip &ip, uint16_t id)
{
    // add to the set
    _processors.emplace(id, ip, processor);
}

/**
 *  Remove a processor from the set
 *  @param  processor       the object that no longer is active
 *  @param  ip              the IP to which it was listening to
 *  @param  id              the query ID in which it was interested
 */
void Inbound::unsubscribe(Processor *processor, const Ip &ip, uint16_t id)
{
    // remove from the set
    _processors.erase(std::make_tuple(id, ip, processor));
    
    // if no other processors are left
    if (_processors.empty()) close();
}
    
/**
 *  End of namespace
 */
}

