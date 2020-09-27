/**
 *  CAA.h
 *
 *  If you have a Record object that holds an CAA record, you can use
 *  this class to extract the properties from it.
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
#include "extractor.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class CAA : public Extractor
{
private:
    /**
     *  The first byte holds the flags
     *  @var uint8_t
     */
    uint8_t _flags = 0;
    
    /**
     *  The tag name
     *  @var char[]
     */
    char _tag[16];
    
    /**
     *  The actual property value (dynamically allocated)
     *  @var char *
     */
    char *_property = nullptr;
    
    /**
     *  Size of the property
     *  @var size_t
     */
    size_t _size = 0;


public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    CAA(const Response &response, const Record &record) : 
        Extractor(record, ns_t_caa, 3)
    {
        // get input data
        auto *buffer = _record.data();
        size_t size = _record.size();
        
        // caa record first have a flag
        _flags = buffer[0];
        
        // followed by the size of the record (property size can be calculated now too)
        size_t tagsize = buffer[1];
        size_t propsize = size - tagsize - 2;
        
        // if tag-size valid?
        if (tagsize < 1 || tagsize > 15) throw std::runtime_error("invalid tagsize");
        
        // copy the tag data + end with an empty '\0' character
        memcpy(_tag, buffer + 2, tagsize);
        
        // allocate enough bytes
        _property = new char[propsize + 1];
        
        // copy property data
        memcpy(_property, buffer + 2 + tagsize, propsize);
        
        // remaining null character
        _tag[tagsize] = '\0';
        _property[propsize] = '\0';
        
        // remember property size
        _size = propsize;
    }
    
    /**
     *  Destructor
     */
    virtual ~CAA()
    {
        // deallocate memory
        delete[] _property;
    }
    
    /**
     *  Was the critical flag set?
     *  @return bool
     */
    bool critical() const
    {
        return _flags == 128;
    }
    
    /**
     *  Retrieve the tag name
     *  @return const char *
     */
    const char *tag() const
    {
        return _tag;
    }
    
    /**
     *  Retrieve the property name
     *  @return const char *
     */
    const char *property() const
    {
        return _property;
    }
};
    
/**
 *  End of namespace
 */
}

