/**
 *  Extractor.h
 *
 *  When you have a Record object, and you want to extract informataion
 *  from the raw response data, you need to use extra objects to do so.
 *  For example, the A or AAAA classes to extract the IP address, and
 *  the MX class to extract the priority and domain name. All these
 *  classes use the Extractor as their base.
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
#include "record.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Canonicalizer;

/**
 *  Class definition
 */
class Extractor
{
protected:
    /**
     *  The original record
     *  @var Record
     */
    const Record &_record;

protected:
    /**
     *  The constructor is protected because only derived classes like A, 
     *  AAAA, CNAME, MX, etc should be public constructable.
     *  @param  record  the record from which data is to be extracted
     *  @param  type    the required type
     *  @param  size    the minimal required size
     *  @throws std::runtime_error
     */
    Extractor(const Record &record, ns_type type, size_t size) : _record(record)
    {
        // must be of the right type
        if (record.type() != type) throw std::runtime_error("type mismatch / wrong record type");
        
        // check size of the record
        if (record.size() < size) throw std::runtime_error("record too small");
    }

    /**
     *  May not be copied to user-space (because a reference to _record is stored)
     *  @param  other
     */
    Extractor(const Extractor &other) = delete;
    Extractor& operator=(const Extractor &other) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Extractor() = default;
    
public:
    /**
     *  The name of the record
     *  @return const char *
     */
    const char *name() const
    {
        return _record.name();
    }
    
    /**
     *  The type of record
     *  @return uint16_t
     */
    uint16_t type() const
    {
        return _record.type();
    }
    
    /**
     *  The dns-class (normally internet)
     *  @return uint16_t
     */
    uint16_t dnsclass() const
    {
        return _record.dnsclass();
    }
    
    /**
     *  The TTL (time-to-live) of this record
     *  @return uint32_t
     */
    uint32_t ttl() const
    {
        return _record.ttl();
    }
    
    /**
     *  Write the rdata to a canonicalized form
     *  This method is used internally by the library to check DNSSEC signatures
     *  @param  canonicalizer
     *  @return bool
     */
    virtual bool rdata(Canonicalizer &canonicalizer) const;
};
    
/**
 *  End of namespace
 */
}

