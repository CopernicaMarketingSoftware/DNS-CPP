/**
 *  TempFcntl.h
 *
 *  Helper class to temporarily add or remove flags from a filedescriptor
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
#include <fcntl.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class FcntlFlags
{
private:
    /**
     *  The socket that is being operator on
     *  @var int
     */
    int _fd;
    
    /**
     *  The original flags
     *  @var int
     */
    int _flags;
    
public:
    /**
     *  Constructor
     *  @param  fd
     *  @throws std::runtime_error
     */
    FcntlFlags(int fd) : _fd(fd), _flags(fcntl(_fd, F_GETFL, 0))
    {
        // leap out on error
        if (_flags < 0) throw std::runtime_error("failed to retrieve fcntl flags");
    }
    
    /**
     *  Destructor
     */
    virtual ~FcntlFlags() = default;
    
    /**
     *  Add a new flag
     *  @param  option
     *  @return bool
     */
    bool add(int option)
    {
        // add the option
        return fcntl(_fd, F_SETFL, _flags |= option) == 0;
    }
    
    /**
     *  Remove an option
     *  @param  option
     *  @return bool
     */
    bool remove(int option)
    {
        // remove the option
        return fcntl(_fd, F_SETFL, _flags &= ~option) == 0;
    }
};
    
/**
 *  End of namespace
 */
}

