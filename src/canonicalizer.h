/**
 *  Canonicalizer.h
 * 
 *  When a record-set is signed, we first need to canonicalize the 
 *  records and turn it into a string. For this string the signature
 *  can be calculcated and/or verified.
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
#include <algorithm>
#include <arpa/inet.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Canonicalizer
{
private:
    /**
     *  The input buffer
     *  @var unsigned char *
     */
    unsigned char *_buffer;
    
    /**
     *  Allocated size of the buffer
     *  @var size_t
     */
    size_t _allocated;
    
    /**
     *  Filled size of the buffer
     *  @var size_t
     */
    size_t _size;
    
    
    /**
     *  Make sure that there is plenty of space
     *  @param  required        required number of bytes
     *  @return bool
     */
    bool reserve(size_t required)
    {
        // the amount of bytes to grow
        size_t grow = std::max(size_t(4096), required);
        
        // reallocate
        auto *newbuffer = (unsigned char *)realloc(_buffer, _allocated + grow);
        
        // check for failure
        if (newbuffer == nullptr) return false;
        
        // we have a new buffer
        _buffer = newbuffer;
        
        // we have more bytes not
        _allocated += grow;
        
        // done
        return true;
    }

protected:
    /**
     *  Size of the buffer
     *  @return size_t
     */
    size_t size() const
    {
        // expose the size
        return _size; 
    }
    
    /**
     *  Restore a previous size
     *  This is useful if you want to rollback some bytes that were added
     *  Only works for sizes smaller than the current size
     *  @param  size
     */
    void restore(size_t size)
    {
        // addign the new size
        _size = std::min(size, _size);
    }
    
    
public:
    /**
     *  Constructor
     *  @throws std::runtime_error
     */
    Canonicalizer() :
        _buffer((unsigned char *)malloc(4096)),
        _allocated(4096),
        _size(0)
    {
        // leap out on failure
        if (_buffer == nullptr) throw std::runtime_error("allocation failure");
    }
    
    /**
     *  Copy constructor
     *  @param  that
     */
    Canonicalizer(const Canonicalizer &that) :
        _buffer((unsigned char *)malloc(that._allocated)),
        _allocated(that._allocated),
        _size(that._size)
    {
        // leap out on failure
        if (_buffer == nullptr) throw std::runtime_error("allocation failure");
        
        // fill the buffer
        memcpy(_buffer, that._buffer, _size);
    }
    
    /**
     *  Move constructor
     *  @param  that
     */
    Canonicalizer(Canonicalizer &&that) :
        _buffer(that._buffer),
        _allocated(that._allocated),
        _size(that._size)
    {
        // invalidate the other object
        that._buffer = nullptr;
        that._allocated = 0;
        that._size = 0;
    }
    
    /**
     *  Destructor
     */
    virtual ~Canonicalizer()
    {
        // deallocte the buffer
        if (_buffer) free(_buffer);
    }
    
    /**
     *  Add a single byte
     *  @param  byte        byte to add
     *  @return bool
     */
    bool add8(uint8_t byte)
    {
        // ensure there is enough space
        if (_allocated <= _size && !reserve(1)) return false;
        
        // add the byte
        _buffer[_size++] = byte;
        
        // done
        return true;
    }
    
    /**
     *  Add a number that consists of two bytes
     *  @param  value       value to add
     *  @return bool
     */
    bool add16(uint16_t value)
    {
        // ensure there is enough space
        if (_allocated -_size < sizeof(value) && !reserve(sizeof(value))) return false;
        
        // convert value to network byte order
        value = htons(value);
        
        // copy to the buffer
        memcpy(_buffer + _size, &value, sizeof(value));
        
        // we have two more bytes
        _size += sizeof(value);
        
        // done
        return true;
    }
    
    /**
     *  Add a number that consists of four bytes
     *  @param  value       value to add
     *  @return bool
     */
    bool add32(uint32_t value)
    {
        // ensure there is enough space
        if (_allocated -_size < sizeof(value) && !reserve(sizeof(value))) return false;
        
        // convert value to network byte order
        value = htonl(value);
        
        // copy to the buffer
        memcpy(_buffer + _size, &value, sizeof(value));
        
        // we have four more bytes
        _size += sizeof(value);
        
        // done
        return true;
    }
    
    /**
     *  Add a binary string to the buffer
     *  @param  data        data to add
     *  @param  size        size of the data
     *  @return bool
     */
    bool add(const unsigned char *data, size_t size)
    {
        // ensure there is enough space
        if (_allocated -_size < size && !reserve(size)) return false;
        
        // copy the data
        memcpy(_buffer + _size, data, size);
        
        // more data is available
        _size += size;
        
        // done
        return true;
    }
};
    
/**
 *  End of namespace
 */
}

