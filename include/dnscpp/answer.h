/**
 *  Answer.h
 * 
 *  Helper class that can be used to extract an answer from a respoonse.
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
class Answer : public Record
{
public:
    /**
     *  Constructor
     *  @param  response    the full response
     *  @param  index       the question-number
     */
    Answer(const Response &response, size_t index = 0) : 
        Record(response, ns_s_an, index) {}
        
    /**
     *  Destructor
     */
    virtual ~Answer() = default;
};
    
/**
 *  End of namespace
 */
}

