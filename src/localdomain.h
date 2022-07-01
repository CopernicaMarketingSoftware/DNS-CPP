/**
 *  LocalDomain.h
 * 
 *  Class that is responsible for finding out the local domainname,
 *  as is described in man resolv.conf(5), used for the search path
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2022 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class LocalDomain
{
private:
    /** 
     *  Buffer large enough to hold the hostname (+1, as that guarantees its 0-terminated)
     *  @var char[]
     */
    char _buffer[HOST_NAME_MAX + 1];

    /**
     *  Pointer to the first dot in the domain
     *  @var const char *
     */
    const char *_firstdot;

public:
    /**
     *  Constructor
     */
    LocalDomain()
    {
        // fill the buffer with the hostname
        gethostname(_buffer, HOST_NAME_MAX);

        // pointer to the end of the buffer
        char *lastchar = _buffer+strlen(_buffer);
        
        // remove trailing dot
        if (*lastchar == '.') *lastchar = '\0';
        
        // we need the first dot for use in our members
        _firstdot = strchr(_buffer, '.');
    }
    
    /**
     *  Destructor
     */
    virtual ~LocalDomain() = default;
    
    /**
     *  Cast to a const char *
     *  @return const char *
     */
    operator const char * () const
    {
        // From the man-page:
        // "By default, the search list contains one entry, the local domain name. It is determined from 
        // the local hostname returned by gethostname(2); the local domain name is taken to be everything
        // after the first '.'.  Finally, if the hostname does not contain a '.', the root domain is 
        // assumed as the local domain name."

        // if there is no dot, we need the root-domain
        if (_firstdot == nullptr) return "";
        
        // return everything after the first dot
        return _firstdot + 1;
    }
};
    
/**
 *  End of namespace
 */
}

