/**
 *  RandomBits.h
 * 
 *  Class to create a random ID that can be used to identify a request
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
class RandomBits
{
private:
    /**
     *  Internally we implement this by fetching the current time
     *  @var struct timeval
     */
  struct timeval _tv;

public:
    /**
     *  Constructor
     */
    RandomBits()
    {
        // get the current time
        gettimeofday(&_tv, nullptr);
    }
    
    /**
     *  Destructor
     */
    virtual ~RandomBits() = default;
    
    /**
     *  Cast to a uint16_t
     *  @return uint16_t
     */
    operator uint16_t () const
    {
        // combine the members
        return (_tv.tv_sec << 8) ^ _tv.tv_usec;
    }
};
    
/**
 *  End of namespace
 */
}
