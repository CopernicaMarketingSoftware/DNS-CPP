/**
 *  Record.h
 * 
 *  Class to extract a single record from a message.
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
#include "message.h"
#include <string.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Record
{
protected:
    /**
     *  The structure of libresolv with info about the record
     *  @var ns_rr
     */
    ns_rr _record;

public:
    /**
     *  Constructor
     *  @param  message         the message from which the record should be extracted
     *  @param  section         the section to extract the record from
     *  @param  index           the record-number inside the section
     *  @throws std::runtime_error
     */
    Record(const ns_msg *message, ns_sect section, int index)
    {
        // parse the buffer (we cast to non-const because that is what libs oddly enough needs)
        if (ns_parserr((ns_msg *)message, section, index, &_record) == 0) return;
        
        // report an error
        throw std::runtime_error("failed to parse record");
    }

    /**
     *  Constructor
     *  @param  message         the message from which the record should be extracted
     *  @param  section         the section to extract the record from
     *  @param  index           the record-number inside the section
     *  @throws std::runtime_error
     */
    Record(const Message &message, ns_sect section, int index) : Record(message.handle(), section, index) {}
    
    /**
     *  Copy constructor 
     *  @param  that            object to copy
     */
    Record(const Record &that)
    {
        // copy the structure
        memcpy(&_record, &that._record, sizeof(ns_rr));
    }
    
    /**
     *  Destructor
     */
    virtual ~Record() = default;
    
    /**
     *  The name of the record
     *  @return const char *
     */
    const char *name() const
    {
        return ns_rr_name(_record);
    }
    
    /**
     *  The type of record
     *  @return uint16_t
     */
    uint16_t type() const
    {
        return ns_rr_type(_record);
    }
    
    /**
     *  The dns-class (normally internet)
     *  @return uint16_t
     */
    uint16_t dnsclass() const
    {
        return ns_rr_class(_record);
    }
    
    /**
     *  The TTL (time-to-live) of this record
     *  @return uint32_t
     */
    uint32_t ttl() const
    {
        return ns_rr_ttl(_record);
    }
    
    /**
     *  The raw message data
     * 
     *  You normally should not access the result data directly. It is
     *  better to use a class like A, AAAA, MX, CNAME, etc to extract
     *  the actual result data from the record.
     * 
     *  @return const char *
     */
    const unsigned char *data() const
    {
        return ns_rr_rdata(_record);
    }
    
    /**
     *  Size of the raw message data
     *  @return uint16_t
     */
    uint16_t size() const
    {
        return ns_rr_rdlen(_record);
    }
};
    
/**
 *  End of namespace
 */
}

