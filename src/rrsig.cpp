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
RRSIG::RRSIG(const Response &response, const Record &record) : Extractor(response, record) 
{
    // we require at least 18 bytes for fixed fields
    if (_record.size() < 18) throw std::runtime_error("RRSIG field is too small");
    
    // where does the signer begin
    auto *signer = record.data() + 18;

    // and parse the name from the data
    auto processed = ns_name_uncompress(response.data(), response.end(), signer, _signer, MAXDNAME);
    
    // leap out if the name could not be decompressed
    if (processed < 0) throw std::runtime_error("failed to extract signer");

    // the signature starts right after the name
    _signature = signer + processed;
}
    
/**
 *  Check if the signature is associated with a certain record
 *  This method does not verify the signature, it just checks if all
 *  the properties of the record and the signature are the same
 *  @param  record
 *  @return bool
 */
bool RRSIG::covers(const Record &record) const
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
    
/**
 *  End of namespace
 */
}

