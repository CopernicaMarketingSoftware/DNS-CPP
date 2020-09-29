/**
 *  RRSIG.cpp
 *  
 *  Implementation file for the RRSIG class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/rrsig.h"
#include "zonename.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  The constructor
 *  @param  response
 *  @param  record
 *  @throws std::runtime_error
 */
RRSIG::RRSIG(const Response &response, const Record &record) : 
    Extractor(record, TYPE_RRSIG, 18),
    _signer(response, _record.data() + 18) {}
    
/**
 *  Check if the signature is associated with a certain record
 *  This method does not verify the signature, it just checks if all
 *  the properties of the record and the signature are the same
 *  @param  record
 *  @return bool
 */
bool RRSIG::covers(const Record &record) const
{
    // prevent exceptions (ZoneName could throw)
    try
    {
        // must be from the same class
        if (dnsclass() != record.dnsclass()) return false;
        
        // ttl's must also match
        if (ttl() != record.ttl()) return false;
        
        // types must be the same
        if (typeCovered() != record.type()) return false;
        
        // get the zonename of the original record
        if (ZoneName(record.name()) != signer()) return false;
        
        // no reason to reject
        return true;
    }
    catch (...)
    {
        // a pass error occured
        return false;
    }
}
    
/**
 *  End of namespace
 */
}

