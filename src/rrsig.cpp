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
#include "../include/dnscpp/type.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/rrsig.h"
#include "zonename.h"
#include <ostream>
#include <iomanip>

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
 *  Write to a stream
 *  @param  stream
 *  @param  signature
 *  @return std::ostream
 */
std::ostream &operator<<(std::ostream &stream, const RRSIG &sig)
{
    // print the basic properties
    stream
        << (int)sig.typeCovered()  << " "
        << (int)sig.algorithm()  << " "
        << (int)sig.labels()  << " "
        << (int)sig.originalTtl() << " ";

    // print the timestamps dig-style
    sig.printformattedtime(stream, sig.validUntil()) << " ";
    sig.printformattedtime(stream, sig.validFrom()) << " ";

    // print the rest of the properties
    stream << (int)sig.keytag() << " " << sig.signer() << " ";

    // print the signature
    // it should be base64-encoded a-la `dig`-style, but we don't have such a
    // function in the library
    stream << "<signature of length " << sig.size() << ">";

    // done
    return stream;
}

/**
 *  End of namespace
 */
}
