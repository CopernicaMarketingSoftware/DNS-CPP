/**
 *  Compressor.h
 *
 *  Helper class for compressing domain names in a message that is sent
 *  to a resolver
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
 *  Class definition
 */
class Compressor
{
private:
    /**
     *  Set of pointers inside the message that point to domain names
     *  @var unsigned char[]
     */
    const unsigned char *_dnptrs[20];

public:
    /**
     *  Constructor
     *  @param  buffer      buffer into which the compressor can write
     */
    Compressor(unsigned char *buffer)
    {
        // install the pointers
        _dnptrs[0] = buffer;
        _dnptrs[1] = nullptr;
    }
    
    /**
     *  No copying
     *  @param  that
     */
    Compressor(const Compressor &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Compressor() = default;
    
    /**
     *  Add a domain to the compressed buffer
     *  @param  name        name of the domain to add
     *  @param  buffer      position in the buffer where the name must be written to
     *  @param  size        remaining size in the buffer
     *  @return size_t      number of bytes written to the buffer
     */
    ssize_t add(const char *name, unsigned char *buffer, size_t size)
    {
        // calculate pointer to the last record in _dnsptrs
        auto *lastdnptr = _dnptrs + sizeof(_dnptrs) / sizeof(_dnptrs[0]);        
        
        // do the compression
        return ns_name_compress(name, buffer, size, _dnptrs, lastdnptr);
    }
};
    
/**
 *  End of namespace
 */    
}


