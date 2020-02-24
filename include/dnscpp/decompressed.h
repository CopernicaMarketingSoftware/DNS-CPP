/**
 *  Decompressed.h
 * 
 *  A decompressed name from a message. The DNS protocol uses a (simple)
 *  compression algorithm to bring back the number of bytes that are used 
 *  inside a message. When a hostname is stored inside a message, it 
 *  could be compressed. This class takes care of decompressing it.
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
class Decompressed
{
private:
    /**
     *  The decompressed name
     *  @var char[]
     */
    char _name[MAXDNAME];
    
    /**
     *  The number of bytes that it originally occupied
     *  @var ssize_t
     */
    ssize_t _bytes;
    
public:
    /**
     *  Constructor
     *  You need to pass the begin and end of the full response buffer (these are
     *  the first two parameters) because the compression algorithm could point
     *  to locations elsewhere in the message. The first parameter ("data")
     *  should point to the beginning of the to-be-extracted domain name
     *  @param  begin       begin of the full response buffer
     *  @param  end         end of the full response buffer
     *  @param  data        the data buffer where the name begins
     *  @throws std::runtime_error
     */
    Decompressed(const unsigned char *begin, const unsigned char *end, const unsigned char *data) :
        _bytes(ns_name_uncompress(begin, end, data, _name, MAXDNAME))
    {
        // check for success
        if (_bytes < 0) std::runtime_error("failed to decompress name");

        // the ns_name_uncompress() method has special handling for the root-domain which we want to roll back
        if (_name[0] == '.') _name[0] = '\0';
    }

    /**
     *  Constructor
     *  You need to pass in the full response because the compression
     *  algorithm could use pointers to other records inside the same message
     *  @param  response        the full response object
     *  @param  data            the data buffer
     *  @throws std::runtime_error
     */
    Decompressed(const Response &response, const unsigned char *data) :
        Decompressed(response.data(), response.end(), data) {}
    
    /**
     *  Destructor
     */
    virtual ~Decompressed() = default;
    
    /**
     *  Cast to a const char *
     *  @return const char *
     */
    operator const char * () const
    {
        // expose the name
        return _name;
    }
    
    /**
     *  The extracted name
     *  @return const char *
     */
    const char *name() const
    {
        // expose the name
        return _name;
    }
    
    /**
     *  Size of the name
     *  @return size_t
     */
    size_t size() const
    {
        // the name is null-terminated, so we can calculate this
        return strlen(_name);
    }
    
    /**
     *  Number of bytes that it consumed inside the message
     *  This could be less than the size of the name if it did indeed use compression
     *  @return size_t
     */
    size_t consumed() const
    {
        // this information was stored
        return _bytes;
    }
};
    
/**
 *  End of namespace
 */
}

