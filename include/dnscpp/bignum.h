/**
 *  Bignum.h
 * 
 *  Inside the data-part of some DNS records (especially the ones used 
 *  for DNSSEC, like the DNSKEY records) big-numbers are stored. These
 *  are numbers that are much bigger than can be stored in 32 of 64 bits,
 *  and for which no standard C++ data type is available. Cryptographic
 *  libraries however (like openssl) do have support for such numbers.
 *
 *  But because DNS-CPP tries to be crypto-lib-independent, we do not
 *  directly call these libraries. Instead, this Bignum class is supplied
 *  that just allows raw access to the underlying data of the big number.
 *  It is up to the caller to pass this on to openssl or a different
 *  library that can deal with big data.
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
namespace DNS {

/**
 *  Class definiiton
 */
class Bignum
{
private;
    /**
     *  Pointer to the data holding the number
     *  @var void *
     */
    const void *_data;
    
    /**
     *  Size of the data
     *  @var size_t
     */
    size_t _size;

protected:
    /**
     *  Protected constructor because you normally do not have to
     *  directly construct instances yourself, but use a derived class
     *  like XXX or XXX
     *  @param  buffer
     *  @param  size
     */
    Bignum(const void *data, size_t size) : _data(data), _size(size) {}

public:
    /**
     *  Destructor
     */
    virtual ~Bignum() = default;
    
    /**
     *  Access to the raw data
     *  @return void *
     */
    const void *data() const { return _data; }
    
    /**
     *  Size of the raw data
     *  @return size_t
     */
    size_t size() const { return _size; }
};
    
/**
 *  End of namespace
 */
}

