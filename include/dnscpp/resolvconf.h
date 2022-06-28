/**
 *  ResolvConf.h
 * 
 *  Class for parsing the/a /etc/resolve.conf file holding the 
 *  configuration options of the resolver.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <vector>
#include <resolv.h>
#include "ip.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class ResolvConf
{
private:
    /**
     *  The detected nameservers
     *  @var std::vector<Ip>
     */
    std::vector<Ip> _nameservers;

    /**
     *  The detected search paths
     *  @var std::vector<std::string>
     */
    std::vector<std::string> _searchpaths;

    /**
     *  Timeout as specified in the conf (this is the interval between attempts)
     *  @var size_t
     */
    size_t _timeout = RES_TIMEOUT;

    /**
     *  How many attempts to make 
     *  @var size_t
     */
    size_t _attempts = RES_DFLRETRY;

    /**
     *  Rotate, see man resolvconf. This indicates if the nameservers should be tried in-order
     *  or if the load can be distributed among them.
     *  @var bool
     */
    bool _rotate = false;

    /**
     *  Helper method to parse lines
     *  @param  line        the line to parse (must already be trimmed)
     *  @param  size        size of the line
     *  @throws std::runtime_error
     */
    void parse(const char *line, size_t size);
    
    /**
     *  Parse a line holding a nameserver
     *  @param  line        the value to parse
     *  @param  size        size of the line
     *  @throws std::runtime_error
     */
    void nameserver(const char *line, size_t size);
    
    /**
     *  Add the local domain
     *  @param  line        the value to parse
     *  @param  size        size of the line
     *  @throws std::runtime_error
     */
    void domain(const char *line, size_t size);
    
    /**
     *  Add a search path
     *  @param  line        the value to parse
     *  @param  size        size of the line
     *  @throws std::runtime_error
     */
    void search(const char *line, size_t size);

    /**
     *  Add an options line
     *  @param  line        the value to parse
     *  @param  size        size of the line
     *  @throws std::runtime_error
     */
    void options(const char *line, size_t size);

    /**
     *  Add an option
     *  @param  option  
     *  @param  size
     */
    void option(const char *option, size_t size);

public:
    /**
     *  Constructor
     *  @param  filename            the file to parse
     *  @param  strict              run in strict mode (do not allow any unsupported or unrecognized data
     *  @throws std::runtime_error
     */
    ResolvConf(const char *filename = "/etc/resolv.conf", bool strict = false);

    /**
     *  Destructor
     */
    virtual ~ResolvConf() = default;
    
    /**
     *  Number of nameservers
     *  @return size_t
     */
    size_t nameservers() const { return _nameservers.size(); }
    
    /**
     *  Get the IP address of one of the nameservers
     *  @param  index
     *  @return Ip
     */
    const Ip &nameserver(size_t index) const { return _nameservers[index]; }

    /**
     *  Number of searchpaths
     *  @return size_t
     */
    size_t searchpaths() const { return _searchpaths.size(); }

    /**
     *  Get a specific search path
     *  @param  index
     *  @return const char *
     */
    const char *searchpath(size_t index) const { return _searchpaths[index]; }

    /**
     *  Whether or not the 'rotate' option is set in the resolve conf
     *  @return bool
     */
    bool rotate() const { return _rotate; }
    
    /**
     *  Timeout of a request (this is the time until the next dgram is sent, but also
     *  the time until we consider a request failed)
     *  @return size_t
     */
    size_t timeout() const { return _timeout; }

    /**
     *  Number of attempts to make before giving up.
     *  @return size_t
     */
    size_t attempts() const { return _attempts; }
};
    
/**
 *  End of namespace
 */
}

