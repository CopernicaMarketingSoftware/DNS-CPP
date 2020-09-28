/**
 *  Hosts.h
 * 
 *  Class that parses the /etc/hosts file and stores all ip-to-host
 *  and host-to-ip information in that file
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
#include <map>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Hosts
{
private:
    /**
     *  Custom comparison object used by the map
     */
    struct HostnameCompare
    {
        /**
         *  The actual comparison function
         *  @param  hostname1
         *  @param  hostname2
         *  @return bool
         */
        bool operator()(const char *hostname1, const char *hostname2) const { return strcasecmp(hostname1, hostname2) < 0; }
    };

    /**
     *  All the hostnames found
     *  @var std::list
     */
    std::list<std::string> _hostnames;

    /**
     *  Map of hostnames to IP addresses
     *  @var std::multimap
     */
    std::multimap<const char *,Ip,HostnameCompare> _host2ip;
    
    /**
     *  Map the other way around, from IP to hostname
     *  @var std::multimap
     */
    std::multimap<Ip,const char *> _ip2host;


    /**
     *  Parse a line from the file
     *  @param  line        the line to parse
     *  @param  size        size of the line
     *  @return bool
     */
    bool parse(const char *line, size_t size);

public:
    /**
     *  Constructor
     *  @param  filename        the filename to parse
     *  @throws std::runtime_error
     */
    Hosts(const char *filename = "/etc/hosts");
    
    /**
     *  No copying
     *  @param  that
     */
    Hosts(const Hosts &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Hosts() = default;
    
    /**
     *  Lookup an IP address given a hostname
     *  This method returns nullptr if there is no match
     *  @param  hostname
     *  @return Ip
     */
    const Ip *lookup(const char *hostname) const;
    
    /**
     *  Lookup a hostname given an IP address
     *  This method returns nullptr if there is no match
     *  @param  hostname
     *  @return const char *
     */
    const char *lookup(const Ip &hostname) const;
};
    
/**
 *  End of namespace
 */
}

