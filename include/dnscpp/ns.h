/**
 *  NS.h
 *
 *  If you have a Record object that holds a NS record, you can use
 *  this extra class to extract the value hostname from it.
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
#include "decompressed.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class NS : public Extractor
{
private:
    /**
     *  The nameserver name
     *  @var char[]
     */
    Decompressed _nameserver;

public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the NS
     *  @throws std::runtime_error
     */
    NS(const Response &response, const Record &record) : 
        Extractor(record, ns_t_ns, 0), 
        _nameserver(response, record.data()) {}
    
    /**
     *  Destructor
     */
    virtual ~NS() = default;
    
    /**
     *  The nameserver hostname
     *  @return const char *
     */
    const char *nameserver() const
    {
        return _nameserver;
    }
};
    
/**
 *  End of namespace
 */
}

