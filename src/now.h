/**
 *  Now.h
 * 
 *  Utility class to get the current time
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
#include <sys/time.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Now
{
private:
    /**
     *  THe structure that holds the current time
     *  @var struct timeval
     */
    struct timeval _time;
    
public:
    /**
     *  Constructor
     */
    Now()
    {
        // get the current time
        gettimeofday(&_time, nullptr);
    }
    
    /**
     *  Destructor
     */
    virtual ~Now() = default;
    
    /**
     *  Expose the time as double
     *  @return double
     */
    operator double () const
    {
        return _time.tv_sec + _time.tv_usec * 1000000;
    }
};
    
/**
 *  End of namespace
 */
}

