/**
 *  A.h
 *
 *  If you have a Record object that holds an A record, you can use
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
#include "ip.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class A : public Extractor
{
private:
    /**
     *  The IP address
     *  @var Ip
     */
    Ip _ip;


public:
    /**
     *  The constructor
     *  @param  response
     *  @param  record
     *  @throws std::runtime_error
     */
    A(const Response &response, const Record &record) : Extractor(record, TYPE_A, 4), _ip((struct in_addr *)_record.data()) {}

    /**
     *  The constructor
     *  @param  record
     *  @throws std::runtime_error
     */
    A(const Record &record) : Extractor(record, TYPE_A, 4), _ip((struct in_addr *)_record.data()) {}
    
    /**
     *  Destructor
     */
    virtual ~A() = default;
    
    /**
     *  The IP address
     *  @return Ip
     */
    const Ip &ip() const
    {
        return _ip;
    }
};
    
/**
 *  End of namespace
 */
}

