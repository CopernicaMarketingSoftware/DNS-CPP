/**
 *  Question.h
 * 
 *  Helper class that can be used to extract the original question
 *  that was sent to a nameserver from the response object
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
class Question : public Record
{
public:
    /**
     *  Constructor
     *  @param  response    the full response
     *  @param  index       the question-number
     */
    Question(const Response &response, size_t index = 0) : 
        Record(response, ns_s_qd, index) {}
        
    /**
     *  Destructor
     */
    virtual ~Question() = default;
};
    
/**
 *  End of namespace
 */
}

