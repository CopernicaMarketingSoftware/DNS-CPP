/**
 *  RSA.h
 * 
 *  Given a DNSKEY record, the RSA base class can be used to extract
 *  the various elements from the RSA based key (this could be a RSA1,
 *  SHA256, SHA512 and possibly some others.
 * 
 *  See RFC 
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
namespace DNS { namespace PublicKey {

/**
 *  Class definition
 */
class SHA
{
protected:
    /**
     *  The actual public key
     *  @var DNS::DNSKEY
     */
    const DNS::DNSKEY &_key;
    
    /**
     *  Number of initial bytes that should be skipped before the data starts
     *  @return size_t
     */
    size_t skip() const
    {
        // if the first byte contains the size of the exponent, only the first byte 
        // can be skipped, otherwise the size is stored in the first three bytes
        return _key.data()[0] == 0 ? 3 : 1;
    }
    
    /**
     *  The constructor is protected because we should only use this as base class
     *  @param  key     the extracted public key info
     *  @throws std::runtime_error
     */
    SHA(const DNS::DNSKEY &key) : _key(key) 
    {
        // we need at least one byte for the size
        if (_key.size() == 0) throw std::runtime_error("not enough key data");
        
        // do we have enough data for all the to-be-skipped bytes?
        if (_key.size() < skip()) throw std::runtime_error("not enough key data");
        
        // do we have enough room for the exponent?
        if (_key.size() - skip() < exponentSize())  throw std::runtime_error("not enough key data");
    }
    
    /**
     *  Protected destructor
     */
    virtual ~SHA() = default;

    
public:
    /**
     *  The size of the exponent-number of the public key
     *  The "exponent" in a primary key is a very-big-number (much bigger than
     *  what fits in an integer), this method returns the number of bytes the 
     *  integer takes in binary representation.
     *  @return size_t
     */
    size_t exponentSize() const
    {
        // if the first byte is not zero, this is stored in the first byte
        size_t firstbyte = _key.data()[0];
        
        // check if the first byte is sufficient
        if (firstbyte) return firstbyte;
        
        // first byte is zero, so the size is stored in the next two bytes
        return ns_get16(_key.data() + 1);
    }

    /**
     *  The beginning of the exponent-data
     *  @return const unsigned char *
     */
    const unsigned char *exponentData() const
    {
        // expose a raw pointer inside the buffer
        return _key.data() + skip();
    }
    
    /**
     *  The size of the modulo number
     *  Just like the "exponent", this is also a very large number
     *  @return size_t
     */
    size_t moduloSize() const
    {
        // all remaining size
        return _key.size() - skip() - exponentSize();
    }

    /**
     *  Buffer to the beginning of the module data
     *  @return const unsigned char *
     */
    const unsigned char *moduloData() const
    {
        return _key.data() + skip() + exponentSize();
    }
};

/**
 *  End of namespace
 */
}}

