/**
 *  TXT.h
 *
 *  If you have a Record object that holds an TXT record, you can use
 *  this extra class to extract the value from it.
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
class TXT : public Extractor
{
private:
    /**
     *  The actual buffer
     *  @var char *_data;
     */
    char *_data = nullptr;
    
    /**
     *  Size of the data
     *  @var size_t
     */
    size_t _size = 0;

public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the TXT record
     *  @throws std::runtime_error
     */
    TXT(const Response &response, const Record &record) : Extractor(record, ns_t_txt, 0)
    {
        // get input data
        auto *buffer = _record.data();
        size_t size = _record.size();

        // number of parts processed
        size_t parts = 0;
        
        // keep processing parts
        while (_size + parts < size)
        {
            // check the size of current part
            size_t size = buffer[_size + parts];
            
            // skip on empty
            if (size == 0) break;
            
            // update counters
            _size += size;
            parts += 1;
        }

        // allocate enough data
        _data = new char[_size + 1];
        
        // last char must be end-of-string character
        _data[_size] = 0;
        
        // number of data already copied
        size_t copied = 0;
        
        // now copy all the parts
        for (size_t i = 0; i < parts; ++i)
        {
            // size of the next part
            size_t partsize = *(buffer + copied + i);
        
            // copy data
            memcpy(_data + copied, buffer + copied + i + 1, partsize);
            
            // update number of bytes copied
            copied += partsize;
        }
    }

    /**
     *  Destructor
     */
    virtual ~TXT()
    {
        // deallocate memory
        delete[] _data;
    }
    
    /**
     *  Get the value
     *  @return const char *
     */
    const char *data() const { return _data; }
    
    /**
     *  Size of the data
     *  @return size_t
     */
    size_t size() const { return _size; }
};
    
/**
 *  End of namespace
 */
}

