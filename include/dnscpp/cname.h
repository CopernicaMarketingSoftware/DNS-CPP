/**
 *  CNAME.h
 *
 *  If you have a Record object that holds an CNAME record, you can use
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

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class CNAME : public Extractor<ns_t_cname>
{
private:
    /**
     *  The target server name
     *  @var char[]
     */
    char _target[MAXDNAME];

public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    CNAME(const Response &response, const Record &record) : Extractor(response, record)
    {
        // and parse the name from the data
        if (ns_name_uncompress(response.data(), response.end(), record.data(), _target, MAXDNAME) >= 0) return;

        // this is some kind of illegal record
        throw std::runtime_error("invalid record");
    }
    
    /**
     *  Destructor
     */
    virtual ~CNAME() = default;
    
    /**
     *  The target hostname
     *  @return const char *
     */
    const char *target() const
    {
        return _target;
    }
};
    
/**
 *  End of namespace
 */
}

