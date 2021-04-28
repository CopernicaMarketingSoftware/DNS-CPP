/**
 *  Query.h
 * 
 *  Class to create the query that is to be sent to the DNS server
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
#include <arpa/nameser.h>
#include <array>
#include <stdint.h>
#include "bits.h"

/**
 *  Begin of namespace
 */
namespace DNS {

// we advertise that we support 1200 bytes for our response buffer size,
// this is the same buffer size as libresolv seems to use. Their ratio
// is that this limits the risk that dgram message get fragmented,
// which makes the system vulnerable for injection
constexpr size_t EDNSPacketSize = 1200;

/**
 *  Forward declarations
 */
class Question;
class Response;

/**
 *  Class definition
 */
class Query
{
private:
    /**
     *  Buffer that is big enough to hold the entire query
     *  @var unsigned char[]
     */
    std::array<unsigned char, HFIXEDSZ + QFIXEDSZ + MAXCDNAME + 1> _buffer;
    
    /**
     *  Size of the buffer
     *  @var size_t
     */
    size_t _size = 0;

    /**
     *  End of the buffer
     *  This is non-const because we also use it for writing
     *  @return unsigned char *
     */
    unsigned char *end()
    {
        return _buffer.data() + _size;
    }

    /**
     *  End of the buffer
     *  @return unsigned char *
     */
    const unsigned char *end() const
    {
        return _buffer.data() + _size;
    }
    
    /**
     *  Remaining buffer size
     *  @return size_t
     */
    size_t remaining() const
    {
        return _buffer.size() - _size;
    }
    
    /**
     *  Helper function to add a 16bit number
     *  @param  value
     */
    void put16(uint16_t value)
    {
        // add two bytes
        _buffer[_size++] = value >> 8;
        _buffer[_size++] = value;
    }
    
    /**
     *  Helper function to add a 32bit number
     *  @param  value
     */
    void put32(uint32_t value)
    {
        // add four bytes
        _buffer[_size++] = value >> 24;
        _buffer[_size++] = value >> 16;
        _buffer[_size++] = value >> 8;
        _buffer[_size++] = value;
    }

    /**
     *  Does this query contain a specific record as question?
     *  @param  record
     *  @return bool
     */
    bool contains(const Question &record) const;
    
    /**
     *  Helper method to add the edns pseudo-section
     *  The original DNS protocol defined a message format that turned out
     *  to be a little bit too small, especially for DNSSEC, which required
     *  some additional properties and flags to be set. The EDNS specification
     *  solves this by allowing an extra pseudo-record to be added to each
     *  message with room for some additional flags and properties. This method
     *  adds that extra pseudo-section to the query.
     *  @param  dnssec      should we query for dnssec-related data?
     *  @return bool
     */
    bool edns(bool dnssec);

public:
    /**
     *  Constructor
     *  @param  op          the type of operation (normally a regular query)
     *  @param  dname       the domain to lookup
     *  @param  type        record type to look up
     *  @param  bits        bits to include in the query
     *  @param  data        optional data (only for type = ns_o_notify)
     *  @throws std::runtime_error
     */
    Query(int op, const char *dname, int type, const Bits &bits, const unsigned char *data = nullptr);

    /**
     *  Destructor
     */
    virtual ~Query() noexcept = default;
    
    /**
     *  The internal raw binary data
     *  @return const unsigned char *
     */
    const unsigned char *data() const noexcept { return _buffer.data(); }
    
    /**
     *  Size of the request
     *  @return size_t
     */
    size_t size() const
    {
        // expose member
        return _size;
    }
    
    /**
     *  The ID inside this object. Note that this may be zero, unless
     *  the query has been sent out over the wire.
     *  @return uint16_t
     */
    uint16_t id() const noexcept;

    /**
     *  The opcode
     *  @return uint8_t
     */
    uint8_t opcode() const;
    
    /**
     *  Number of questions
     *  @return size_t
     */
    size_t questions() const;
    
    /**
     *  Does a response match with a query, meaning: is the response 
     *  indeed a response to this specific query?
     *  @param  response
     *  @return bool
     */
    bool matches(const Response &response) const;
};
    
/**
 *  End of namespace
 */
}

