/**
 *  Hosts.h
 * 
 *  Class that parses the /etc/hosts file and stores all ip-to-host
 *  and host-to-ip information in that file
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2025 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <map>
#include <list>
#include <memory>

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Forward declarations
 */
class Handler;
class Operation;
class Request;

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
     *  The hostname-list
     */
    using Hostnames = std::list<std::string>;

    /**
     *  All the hostnames found - this member is just here to keep the strings in scope
     *  @var std::list
     */
    std::shared_ptr<Hostnames> _hostnames;

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
     *  Default constructor
     *  This does not yet read the /etc/hosts file, you need to call load() for that
     */
    Hosts() : _hostnames(std::make_shared<Hostnames>()) {}

    /**
     *  Constructor that immediately reads a file
     *  @param  filename        the filename to parse
     *  @throws std::runtime_error
     */
    Hosts(const char *filename);
    
    /**
     *  Destructor
     */
    virtual ~Hosts() = default;
    
    /**
     *  Load a certain file
     *  All lines in the file are merged with the lines already in memory
     *  @param  filename
     *  @return bool
     */
    bool load(const char *filename = "/etc/hosts");
    
    /**
     *  Lookup an IP address given a hostname
     *  This method returns nullptr if there is no match
     *  @param  hostname        hostname to lookup
     *  @param  version         required ip version (0 for no matter)
     *  @return Ip
     */
    const Ip *lookup(const char *hostname, unsigned int version = 0) const;
    
    /**
     *  Lookup a hostname given an IP address
     *  This method returns nullptr if there is no match
     *  @param  hostname
     *  @return const char *
     */
    const char *lookup(const Ip &hostname) const;
    
    /**
     *  Notify a user-space handler about a certain hostname to ip combination
     *  @param  request         the original request
     *  @param  handler         user-space object that should be notified
     *  @param  operation       the operation-pointer that should be passed
     *  @return bool            was the handler called?
     */
    bool notify(const Request &request, Handler *handler, const Operation *operation) const;
    
};
    
/**
 *  End of namespace
 */
}

