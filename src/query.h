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
#include <string.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <stdexcept>
#include "randombits.h"
#include "compressor.h"
#include "../include/dnscpp/question.h"
#include "../include/dnscpp/response.h"

/**
 *  Begin of namespace
 */
namespace DNS {

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
    bool contains(const Question &record) const
    {
        // start of the buffer
        auto *current = _buffer + HFIXEDSZ;
        
        // check all questions
        for (size_t i = 0; i < questions(); ++i)
        {
            // buffer in which we're going to store the name
            char name[MAXDNAME+1];
            
            // get the name
            ssize_t size = ns_name_uncompress(_buffer, end(), current, name, MAXDNAME+1);
            
            // skip on error
            if (size < 0) continue;
            
            // make sure name is not a dot
            if (name[0] == '.') name[0] = '\0';
            
            // update current pointer
            current += size;
            
            // check if we still have room for two uint16's
            if (end() - current < 4) break;
            
            // get the next two shorts
            uint16_t type = ns_get16(current);
            uint16_t dnsclass = ns_get16(current + 2);
            
            // update current pointer
            current += 4;
            
            // now run the tests
            if (type != record.type()) continue;
            if (dnsclass != record.dnsclass()) continue;
            
            // type and class are then same, we just need the same name
            if (ns_samename(name, record.name()) > 0) return true;
        }
            
        // record not found
        return false;
    }
    
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
    bool edns()
    {
        // check if there is enough room
        if (remaining() < 11) return false;

        // the first field for a record is the domain name, this is not in use
        // so we add an empty string (or the root domain, which is the same)
        _buffer[_size++] = 0;
        
        // the type of the pseudo-record is "opt"
        put16(ns_t_opt);
        
        // we advertise that we support 1200 bytes for our response buffer size, 
        // this is the same buffer size as libresolv seems to use. Their ratio
        // is that this limits the risk that dgram message get fragmented,
        // which makes the system vulnerable for injection
        put16(1200);
        
        // extended rcode (0 because the normal rcode is good enough) and the 
        // edns version (also 0 because that is the mose up-to-date version)
        _buffer[_size++] = 0;
        _buffer[_size++] = 0;
        
        // extra flags to say that dnssec is supported
        put16(NS_OPT_DNSSEC_OK);
        
        // the next section contains a key-value list of extra options, but
        // since we do not support such extra options, we have a zero-length list
        put16(0);

        // for simpler access to the header-properties, we use a local variable
        HEADER *header = (HEADER *)_buffer;

        // increment the counter with number of additional sections
        header->arcount = htons(ntohs(header->arcount)+1);
        
        // this was a huge success
        return true;
    }

public:
    /**
     *  Constructor
     *  @param  op          the type of operation (normally a regular query)
     *  @param  dname       the domain to lookup
     *  @param  type        record type to look up
     *  @param  data        optional data (only for type = ns_o_notify)
     *  @throws std::runtime_error
     */
    Query(int op, const char *dname, int type, const unsigned char *data = nullptr) : _size(HFIXEDSZ)
    {
        // check if parameters fit in the header
        if (type < 0 || type > 65535) throw std::runtime_error("invalid type passed to dns query");
        
        // make sure buffer is completely filled with zero's
        memset(_buffer, 0, sizeof(_buffer));
        
        // for simpler access to the header-properties, we use a local variable
        HEADER *header = (HEADER *)_buffer;

        // we need a random id so that it cannot be guessed
        header->id = RandomBits();

        // store the opcode
        header->opcode = op;
        
        // the default behavior is to ask for recursion
        header->rd = 1;
        
        // no error
        header->rcode = ns_r_noerror;
        
        // Perform opcode specific processing
        switch (op) {
        case NS_NOTIFY_OP:
        case QUERY:         break;
        default:            throw std::runtime_error("invalid dns operation");
        }
        
        // we will be compressing data
        Compressor compressor(_buffer);

        // compress the name in the buffer
        auto size = compressor.add(dname, end(), remaining());
        
        // check for success
        if (size < 0) throw std::runtime_error("failed domain name compression");
        
        // update the buffer size
        _size += size;
        
        // add the type and dns class
        put16(type);
        put16(ns_c_in);
        
        // only one record is in the query-part
        header->qdcount = htons(1);
        
        // pseudo-loop to be able to break out
        do
        {
            // done if there is no extra part
            if (op == QUERY || data == nullptr) break;
            
            // make an additional record for completion domain.  */
            size = compressor.add((char *)data, end(), remaining());
            
            // check for success
            if (size < 0) throw std::runtime_error("failed data name compression");
            
            // add the further data
            put16(T_NULL);
            put16(ns_c_in);
            put32(0);
            put16(0);
            
            // one argument was added
            header->arcount = htons(1);
        }
        while (false);
        
        // add the edns-pseudo-section
        edns();
    }
    
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
    uint16_t id() const
    {
        // use a local variable to access properties
        HEADER *header = (HEADER *)_buffer;
        
        // expose the properties
        return ntohs(header->id);
    }
    
    /**
     *  The opcode
     *  @return uint8_t
     */
    uint8_t opcode() const
    {
        // use a local variable to access properties
        HEADER *header = (HEADER *)_buffer;
        
        // expose the properties
        return header->opcode;
    }
    
    /**
     *  Number of questions
     *  @return size_t
     */
    size_t questions() const
    {
        // use a local variable to access properties
        HEADER *header = (HEADER *)_buffer;
        
        // expose the properties
        return ntohs(header->qdcount);
    }
    
    
    /**
     *  Does a response match with a query, meaning: is the response 
     *  indeed a response to this specific query?
     *  @param  response
     *  @return bool
     */
    bool matches(const Response &response)
    {
        // prevent exceptions (parsing a record could fail)
        try
        {
            // the ids must match
            if (response.id() != id()) return false;
            
            // in dynamic update packets there is only a header so we cannot check the content
            if (response.opcode() == ns_o_update && opcode() == ns_o_update) return true;
            
            // the query and response must have the same number of questions
            if (response.questions() != questions()) return false;
            
            // we'll be checking all the questions in the question-section of the response
            // to see if they also appear in the origina query (they should!)
            for (size_t i = 0; i < response.questions(); ++i)
            {
                // get the record from the question-section, and check if the query contains that record
                if (!contains(Question(response, i))) return false;
            }
        
            // there seems to be a match
            return true;
        }
        catch (...)
        {
            // parse error -- we treat this as a no-match
            return false;
        }
    }
};
    
/**
 *  End of namespace
 */
}

