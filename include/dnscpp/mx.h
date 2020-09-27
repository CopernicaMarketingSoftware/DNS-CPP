/**
 *  MX.h
 *
 *  If you have a Record object that holds an MX record, you can use
 *  this extra class to extract the action IP address from it.
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
class MX : public Extractor
{
private:
    /**
     *  The target server name
     *  @var char[]
     */
    Decompressed _target;

public:
    /**
     *  The constructor
     *  @param  response
     *  @param  record
     *  @throws std::runtime_error
     */
    MX(const Response &response, const Record &record) : 
        Extractor(record, ns_t_mx, 2),
        _target(response, record.data() + 2)  {} // first two bytes of the priority are skipped
    
    /**
     *  Destructor
     */
    virtual ~MX() = default;
    
    /**
     *  The priority
     *  @return uint16_t
     */
    uint16_t priority() const
    {
        // extract the value
        return ns_get16(record.data());
    }

    /**
     *  The hostname
     *  @return const char *
     */
    const char *hostname() const
    {
        // return the hostname
        return _target;
    }
};
    
/**
 *  End of namespace
 */
}

