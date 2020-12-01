/**
 *  FakeResponse.h
 * 
 *  Class for creating a reply message that is normally sent by nameservers.
 *  This class is used internally when a query is not necessary because
 *  the answer is already in the /etc/hosts file. We then make a fake reply
 *  ourselves.
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
#include "compressor.h"
#include "randombits.h"
#include "../include/dnscpp/type.h"
#include "../include/dnscpp/request.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class FakeResponse
{
private:
    /**
     *  The buffer that is being filled
     *  @todo check for overflow if user has a ridiculous /etc/hosts file
     *  @var char[]
     */
    unsigned char _buffer[4096];
    
    /**
     *  Size already allocated
     *  @var size_t
     */
    size_t _size;
    
    /**
     *  Compressor to write to the buffer
     *  @var Compressor
     */
    Compressor _compressor;
    
    /**
     *  Compress a domain 
     *  @param  name        the record name
     *  @param  withsize    also store the size in the two bytes right in front
     *  @return bool
     */
    bool compress(const char *name, bool withsize)
    {
        // if we need to store the size too, we need to skip two byte that will later hold the size
        size_t skip = withsize ? 2 : 0;
        
        // we first compress the name
        auto size = _compressor.add(name, _buffer + _size + skip, sizeof(_buffer) - _size - skip);
        
        // check for success
        if (size < 0) return false;
        
        // store the size
        if (withsize) put16(size);
        
        // we have some extra bytes
        _size += size;
        
        // done
        return true;
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
    
public:
    /**
     *  Convenience constructor if the caller had already extracted the question
     *  @param  request     the request for which this is a fake response
     *  @param  question    question extracted from the request
     *  @throws std::runtime_error
     */
    FakeResponse(const Request &request, const Question &question) : _size(sizeof(HEADER)), _compressor(_buffer)
    {
        // make sure buffer is completely filled with zero's
        memset(_buffer, 0, sizeof(HEADER));
        
        // get access to the header (this makes it easier to set properties)
        HEADER *header = (HEADER *)_buffer;

        // we need a random id so that it cannot be guessed
        header->id = htons(request.id());

        // store the opcode
        header->opcode = ns_o_query;
        
        // the default behavior is to ask for recursion (rd), we pretend that recursion 
        // is available (ra), and we set the bit to mark this message an answer (qr),
        // we also mark the data as verified (because this is was "dig" also does, and
        // it makes sense to mark data in /etc/hosts as verified (if we cannot trust
        // our own system, what then?))
        header->rd = request.recursiondesired();
        header->ra = 1;
        header->qr = 1;
        header->ad = 1;
        
        // no error
        header->rcode = ns_r_noerror;

        // only one record is in the query-part
        header->qdcount = htons(1);
        
        // compress the name in the buffer for the question section
        if (!compress(question.name(), false)) throw std::runtime_error("failed domain name compression");

        // add the type and dns class in the question section
        put16(question.type());
        put16(ns_c_in);
    }

    /**
     *  Constructor
     *  @param  request     the request for which this is a fake response
     *  @throws std::runtime_error
     */
    FakeResponse(const Request &request) : FakeResponse(request, Question(request)) {}
    
    /**
     *  No copying
     *  @param  that
     */
    FakeResponse(const FakeResponse &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~FakeResponse() = default;
    
    /**
     *  Expose the data
     *  @return const char *
     */
    const unsigned char *data() const { return _buffer; }
    
    /**
     *  Size of the buffer
     *  @return size_t
     */
    size_t size() const { return _size; }
    
    /**
     *  Method to answer a response section
     *  @param  name        the record name
     *  @param  hostname    the hostname
     *  @return bool
     */
    bool append(const char *name, const char *hostname)
    {
        // add the name
        if (!compress(name, false)) return false;

        // get access to the header (this makes it easier to set properties)
        HEADER *header = (HEADER *)_buffer;

        // update number of answers
        header->ancount = htons(answers() + 1);
        
        // add type, class, ttl and size of the data
        put16(TYPE_PTR);
        put16(ns_c_in);
        put32(0);
        
        // compress the hostname
        return compress(hostname, true);
    }
    
    /**
     *  Method to add an answer
     *  @param  name        the record name
     *  @param  ip          the IP address
     */
    bool append(const char *name, const DNS::Ip &ip)
    {
        // add the name
        if (!compress(name, false)) return false;

        // get access to the header (this makes it easier to set properties)
        HEADER *header = (HEADER *)_buffer;
        
        // update number of answers
        header->ancount = htons(answers() + 1);
        
        // add type, class, ttl and size of the data
        put16(ns_t_a);
        put16(ns_c_in);
        put32(0);
        put16(ip.size());
        
        // store the actual IP address
        memcpy(_buffer + _size, ip.data(), ip.size());
        
        // update size
        _size += ip.size();
        
        // done
        return true;
    }
    
    /**
     *  The number of answers in the message
     *  @return size_t
     */
    size_t answers() const
    {
        // get access to the header (this makes it easier to set properties)
        HEADER *header = (HEADER *)_buffer;
        
        // expose counter in header
        return ntohs(header->ancount);
    }
};
    
/**
 *  End of namespace
 */
}

