/**
 *  Message.h
 * 
 *  Class to parse a response from a nameserver or a request from a client
 *  and to extract the individual properties from it. This is the base
 *  class for the Response and Request classes.
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
#include <stdexcept>
#include <string.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Message
{
private:
    /**
     *  Pointer to the buffer (only if the object is copied around)
     *  @var unsigned char *
     */
    unsigned char *_buffer = nullptr;

    /**
     *  Handle to the message
     *  @var ns_msg
     */
    ns_msg _handle;

protected:
    /**
     *  Constructor
     *  @param  buffer      the raw data that we received
     *  @param  size        size of the buffer
     *  @throws std::runtime_error
     */
    Message(const unsigned char *buffer, size_t size) 
    {
        // try parsing the buffer
        if (ns_initparse(buffer, size, &_handle) == 0) return;
        
        // on failure we report an error
        throw std::runtime_error("failed to parse dns message");
    }
    
    /**
     *  Copy constructor
     *  It is better not to use this too often as it might not be too efficient to copy responses around
     *  @param  that
     *  @throws std::runtime_error
     */
    Message(const Message &that) : _buffer((unsigned char *)malloc(that.size()))
    {
        // buffer should exist
        if (_buffer == nullptr) throw std::runtime_error("failed memory allocation");
        
        // copy the raw data
        memcpy(_buffer, that.data(), that.size());
        
        // try parsing the buffer
        if (ns_initparse(_buffer, that.size(), &_handle) == 0) return;
        
        // on failure we report an error
        throw std::runtime_error("failed to parse dns message");
    }
    
public:
    /**
     *  Destructor
     */
    virtual ~Message()
    {
        // deallocate optional buffer
        if (_buffer) free(_buffer);
    }
    
    /**
     *  Get the internal handle
     *  This is normally not necessary, because you can get access to all
     *  properties via the methods, but it could be useful if you want to
     *  call libresolv functions yourself.
     *  @return ns_msg*
     */
    const ns_msg *handle() const
    {
        // expose member
        return &_handle;
    }
    
    /**
     *  Access to the raw buffer data
     *  @return unsigned char *
     */
    const unsigned char *data() const
    {
        return ns_msg_base(_handle);
    }
    
    /**
     *  Pointer right after the data
     *  @return unsigned char *
     */
    const unsigned char *end() const
    {
        return ns_msg_end(_handle);
    }
    
    /**
     *  Size of the raw message
     *  @return size_t
     */
    size_t size() const
    {
        return ns_msg_size(_handle);
    }
    
    /**
     *  ID of the message
     *  This is used internally by the library to check if it matches with the request
     *  @return uint16_t
     */
    uint16_t id() const
    {
        return ns_msg_id(_handle);
    }
    
    /**
     *  Get access to a flag from the header
     *  @param  flag        the type of flag (see man ns_msg_get_flag()
     *  @return bool
     */
    bool flag(ns_flag flag) const
    {
        return ns_msg_getflag(_handle, flag);
    }
    
    /**
     *  Is this a question or a response
     *  @return bool
     */
    bool question() const { return !flag(ns_f_qr); }
    bool response() const { return !question(); }
    
    /**
     *  Is the answer authoratative? Meaning that it does not come from a cache.
     *  @return bool
     */
    bool authoratative() const { return flag(ns_f_aa); }
    
    /**
     *  Is the message truncated? In that case it is better to use tcp
     *  @return bool
     */
    bool truncated() const { if (rand() % 2 == 0) return true; else return flag(ns_f_tc); }
    
    /**
     *  Is recursion desired (false for responses)
     *  @return bool
     */
    bool recursiondesired() const { return flag(ns_f_rd); }
    
    /**
     *  Is recursion available on this nameserver?
     *  @return bool
     */
    bool recursionavailable() const { return flag(ns_f_ra); }
    
    /**
     *  Is this authentic data (used for dnssec)
     *  @return bool
     */
    bool authentic() const { return flag(ns_f_ad); }
    
    /**
     *  Is checking disabled (used for dnssec)
     *  @return bool
     */
    bool checkingdisabled() const { return flag(ns_f_cd); }
    
    /**
     *  The opcode, see arpa/nameser.h for supported opcodes
     *  @return ns_opcode
     */
    ns_opcode opcode() const { return (ns_opcode)ns_msg_getflag(_handle, ns_f_opcode); }
    
    /**
     *  The result code, see arpa/nameser.h for supported opcodes
     *  A result-code of zero is generally good, other codes indicate an error
     *  @return ns_rcode
     */
    ns_rcode rcode() const;
    
    /**
     *  Number of records in one specific section
     *  @param  section     the type of section
     *  @return uint16_t
     */
    uint16_t records(ns_sect section) const { return ns_msg_count(_handle, section); }

    /**
     *  Number of records of a certain type in a certain section
     *  @param  section     the type of section
     *  @param  type        the record type
     *  @param  dnsclass    the dnsclass
     *  @return uint16_t
     */
    uint16_t records(ns_sect section, uint16_t type, uint16_t dnsclass = ns_c_in);

    /**
     *  Methods to return the number of records in one specific section
     *  To get access to these records, use the Record class (or one of the
     *  derived classes).
     *  @return uint16_t
     */
    uint16_t questions() const { return records(ns_s_qd); }
    uint16_t answers() const { return records(ns_s_an); }
    uint16_t nameservers() const { return records(ns_s_ns); }
    uint16_t additional() const { return records(ns_s_ar); }
    
    
};
    
/**
 *  End of namespace
 */
}
