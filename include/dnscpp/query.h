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

/**
 *  Begin of namespace
 */
namespace DNS {

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
    unsigned char _buffer[HFIXEDSZ + QFIXEDSZ + MAXCDNAME + 1];
    
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
        return _buffer + _size;
    }

    /**
     *  End of the buffer
     *  This is non-const because we also use it for writing
     *  @return unsigned char *
     */
    const unsigned char *end() const
    {
        return _buffer + _size;
    }
    
    /**
     *  Remaining buffer size
     *  @return size_t
     */
    size_t remaining() const
    {
        return sizeof(_buffer) - _size;
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
     *  @return bool
     */
    bool edns();

public:
    /**
     *  Constructor
     *  @param  op          the type of operation (normally a regular query)
     *  @param  dname       the domain to lookup
     *  @param  type        record type to look up
     *  @param  data        optional data (only for type = ns_o_notify)
     *  @throws std::runtime_error
     */
    Query(int op, const char *dname, int type, const unsigned char *data = nullptr);

    /**
     *  No copying
     *  @param that
     */
    Query(const Query &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Query() = default;
    
    /**
     *  The internal raw binary data
     *  @return const unsigned char *
     */
    const unsigned char *data() const
    {
        // expose the buffer
        return _buffer;
    }
    
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
     *  The ID inside this object
     *  @return uint16_t
     */
    uint16_t id() const;
    
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
    bool matches(const Response &response);
};
    
/**
 *  End of namespace
 */
}

