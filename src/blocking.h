/**
 *  Blocking.h
 * 
 *  Helper class to make a socket temporarily blocking again
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
#include "fcntlflags.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Blocking
{
private:
    /**
     *  The file control flags
     *  @var FcntlFlags
     */
    FcntlFlags _flags;

public:
    /**
     *  Constructor
     *  @param  int     file descriptor
     *  @throws std::runtime_error
     */
    Blocking(int fd) : _flags(fd)
    {
        // make the socket blocking by removing the non-blocking flag
        _flags.remove(O_NONBLOCK);
    }
    
    /**
     *  Destructor
     */
    virtual ~Blocking()
    {
        // restore the non-blocking flag
        _flags.add(O_NONBLOCK);
    }
};
    
/**
 *  End of namespace
 */
}
