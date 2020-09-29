/**
 *  Additional.h
 * 
 *  Helper class that can be used to extract a record from the "additional"
 *  section in a message.
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
     *  @param  mess age    the full message
     *  @param  index       the question-number
     */
    Additional(const Message &message, size_t index = 0) : 
        Record(message, ns_s_ar, index) {}
        
    /**
     *  Destructor
     */
    virtual ~Additional() = default;
};
    
/**
 *  End of namespace
 */
}

