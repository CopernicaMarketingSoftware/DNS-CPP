/**
 *  Additional.h
 * 
 *  Helper class that can be used to extract a record from the "additional"
 *  section in a respoonse.
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
#include "record.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Additional : public Record
{
public:
    /**
     *  Constructor
     *  @param  response    the full response
     *  @param  index       the question-number
     */
    Additional(const Response &response, size_t index = 0) : 
        Record(response, ns_s_ar, index) {}
        
    /**
     *  Destructor
     */
    virtual ~Additional() = default;
};
    
/**
 *  End of namespace
 */
}

