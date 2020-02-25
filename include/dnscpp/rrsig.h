/**
 *  RRSIG.h
 *
 *  If you have a Record object that holds an RRSIG record, you can use
 *  this extra class to extract the properties from that RRSIG record.
 *
 *  An RRSIG record is used for DNSSEC. It is sent with each response
 *  and holds the signature of that response. To check whether the
 *  signature is indeed valid you can check if with a public key.
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
#include <arpa/inet.h>
#include "extractor.h"
#include "decompressed.h"
#include "algorithm.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class RRSIG : public Extractor
{
private:
    /**
     *  The signer's name
     *  @var Decompressed
     */
    Decompressed _signer;

    
public:
    /**
     *  The constructor
     *  @param  response
     *  @param  record
     *  @throws std::runtime_error
     */
    RRSIG(const Response &response, const Record &record);
    
    /**
     *  Destructor
     */
    virtual ~RRSIG() = default;
    
    /**
     *  The type of record for which this record holds the signature
     *  @return ns_type
     */
    ns_type typeCovered() const
    {
        return ns_type(ns_get16(_record.data()));
    }
    
    /**
     *  The algorith that is in use
     *  @return ALgorithm
     */
    Algorithm algorithm() const
    {
        return Algorithm(_record.data()[2]);
    }
    
    /**
     *  The number of labels that were signed (if the record name was
     *  www.copernica.com the value is 3. For wildcard records like
     *  *.copernica.com the value is 2.
     *  @return uint8_t
     */
    uint8_t labels() const
    {
        return _record.data()[3];
    }
    
    /**
     *  The original TTL value as it appeared in the zone configuration.
     *  This is the initial TTL value, and not the one that is decrementing
     *  @return uint32_t
     */
    uint32_t originalTtl() const
    {
        return ns_get32(_record.data() + 4);
    }
    
    /**
     *  When does the signature expire? This signature should not be used after this time
     *  @return time_t
     */
    time_t validUntil() const
    {
        return ns_get32(_record.data() + 8);
    }
    
    /**
     *  From when onwards is the signature valid? It should not be used before this time.
     *  @return time_t
     */
    time_t validFrom() const
    {
        return ns_get32(_record.data() + 12);
    }
    
    /**
     *  The key-tag value
     *  @return uint16_t
     */
    uint16_t keytag() const
    {
        return ns_get16(_record.data() + 16);
    }
    
    /**
     *  The signer's name, this is a null-terminated string
     *  @return const char *
     * 
     *  @todo check if this is a parent zone?
     */
    const char *signer() const
    {
        // expose the member
        return _signer;
    }
    
    /**
     *  The actual signature
     *  @return const unsigned char *
     */
    const unsigned char *signature() const
    {
        return _record.data() + 18 + _signer.consumed();
    }
    
    /**
     *  Size of the signature
     *  @return size_t
     */
    size_t size() const
    {
        // number of bytes into the message where the signature starts
        size_t offset = 18 + _signer.consumed();
        
        // calculate the size
        return _record.size() - offset;
    }
    
    /**
     *  Check if the signature is associated with a certain record
     *  This method does not verify the signature, it just checks if all
     *  the properties of the record and the signature are the same
     *  @param  record
     *  @return bool
     */
    bool covers(const Record &record) const;
};
    
/**
 *  End of namespace
 */
}

