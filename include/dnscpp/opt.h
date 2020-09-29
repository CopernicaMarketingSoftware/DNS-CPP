/**
 *  OPT.h
 *
 *  If you have a Record object that holds an OPT record, you can use
 *  this extra class to extract the properties from that OPT record.
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

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class OPT : public Extractor
{
public:
    /**
     *  The constructor
     *  @param  message
     *  @param  record
     *  @throws std::runtime_error
     */
    OPT(const Message &message, const Record &record) : Extractor(record, TYPE_OPT, 0) {}
    
    /**
     *  Destructor
     */
    virtual ~OPT() = default;
    
    /**
     *  Max UDP payload size that the sender accepts for response messages
     *  @return uint16_t
     */
    uint16_t payload() const
    {
        // the dnsclass property is abused for this
        return dnsclass();
    }
    
    /**
     *  The high eight bits that should be added to the rcode of the header
     *  Note that the value is not yet shifted to the right
     *  @return uint32_t
     */
    uint32_t rcode() const
    {
        // we convert the ttl to network-byte-order (big endian), to normalize
        // the order of the bytes, which is byte4|byte3|byte2|byte1, and we
        // need the first byte from the left (which is then on position 4)
        return htonl(ttl()) & 0xff;
    }
    
    /**
     *  The EDNS version number
     *  @return uint32_t
     */
    uint32_t version() const
    {
        // same implementation as rcode(), but this time we need the second byte,
        // which is on position three
        return (htonl(ttl()) & 0xff00) >> 8;
    }
};
    
/**
 *  End of namespace
 */
}

