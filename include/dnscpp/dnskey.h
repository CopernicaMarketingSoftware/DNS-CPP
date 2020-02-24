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
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class DNSKEY : public Extractor<ns_t_dnskey>
{
private:

public:
    /**
     *  Constructor
     *  @param  response    the full response
     *  @param  record      the record holding a key
     *  @throws runtime_error
     */
    DNSKEY(const Response &response, const Record &record) : Extractor(response, record)
    {
        // we need at least four bytes for the flags, protocol and algorithm
        if (record.size() < 4) throw std::runtime_error("DNSKEY record is too small");
    }
    
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
        return byte & 0x1 != 0;
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
        return byte & 0x1 != 0;
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
     *  @return uint8_t
     */
    uint8_t algorithm() const
    {
        return _record.data()[3];
    }
    
    /**
     *  The actual data of the public key
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

