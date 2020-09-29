/**
 *  DNSKEY.h
 * 
 *  Extractor class that can be used for Record objects that contain
 *  a DNSKEY record. With this class you can extract the properties
 *  of the public key.
 * 
 *  The DNSKEY's stored in DNS hold public keys than can be used to
 *  verify the signatures that are stored in RRSIG records.
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
#include "extractor.h"
#include "algorithm.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class DNSKEY : public Extractor
{
private:

public:
    /**
     *  Constructor
     *  @param  response    the full response
     *  @param  record      the record holding a key
     *  @throws runtime_error
     */
    DNSKEY(const Response &response, const Record &record) : Extractor(record, TYPE_DNSKEY, 4) {}
    
    /**
     *  Destructor
     */
    virtual ~DNSKEY() = default;
    
    /**
     *  Is this a zonekey? If this is not the case the public key has no meaning 
     *  for DNSSEC and should not be used for verifying RRSIG's
     *  @return bool
     */
    bool zonekey() const
    {
        // we need bit seven of the first byte
        uint8_t byte = _record.data()[0];
        
        // check the seventh bit
        return (byte & 0x1) != 0;
    }
    
    /**
     *  Is the a secure entry point bit set?
     *  @return bool
     */
    bool sep() const
    {
        // we need the last bit of the second byte
        uint8_t byte = _record.data()[1];
        
        // check the seventh bit
        return (byte & 0x1) != 0;
    }
    
    /**
     *  Get the protocol number -- this must be 3 otherwise the key should 
     *  not be used for RRSIG verification
     *  @return uint8_t
     */
    uint8_t protocol() const
    {
        return _record.data()[2];
    }
    
    /**
     *  The security algorithm that this public key is meant for
     *  @return Algorithm
     */
    Algorithm algorithm() const
    {
        return Algorithm(_record.data()[3]);
    }
    
    /**
     *  The key-tag. This is a sort of key-identifier that should match the
     *  keytag in the RRSIG record. If it does not match, it is pointless
     *  to even try validating the signature
     *  @return uint32_t
     */
    uint16_t keytag() const;
    
    /**
     *  The actual data of the public key
     * 
     *  The meaning of this data depends on the value of the algorithm. 
     *  There are other classes in DNS-CPP to extract the actual value 
     *  from this data. Example classes that can be used to analyze this 
     *  data are for example:
     * 
     *      -   DNS::Crypto::RSA256
     *      -   DNS::Crypto::RSA512
     * 
     *  @return const char
     */
    const unsigned char *data() const
    {
        return _record.data() + 4;
    }
    
    /**
     *  Size of the public key
     *  @return size_t
     */
    size_t size() const
    {
        return _record.size() - 4;
    }
};
    
/**
 *  End of namespace
 */
}

