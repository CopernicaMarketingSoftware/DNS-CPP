/**
 *  The configuration
 * 
 *  Object that holds the nameservers, loaded from /etc/resolv.conf and
 *  the /etc/hosts file settings
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2025 Copernica BV
 */

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Class definition
 */
class Config
{
private:
    /**
     *  The IP addresses of the servers that can be accessed
     *  @var std::vector<Ip>
     */
    std::vector<Ip> _nameservers;

    /**
     *  The IP addresses of the servers that can be accessed
     *  @var std::vector<Ip>
     */
    std::vector<std::string> _searchpaths;
    
    /**
     *  The contents of the /etc/hosts file
     *  @var Hosts
     */
    Hosts _hosts;

public:
    /**
     *  Default constructor
     *  @param  settings
     */
    Config(const ResolvConf &settings)
    {
        // construct the nameservers and search paths
        for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(settings.nameserver(i));
        for (size_t i = 0; i < settings.searchpaths(); ++i) _searchpaths.emplace_back(settings.searchpath(i));

        // we also have to load /etc/hosts
        if (!_hosts.load()) throw std::runtime_error("failed to load /etc/hosts");
    }

    /**
     *  Default constructor
     */
    Config() = default;

    /**
     *  Expose the /etc/hosts file
     *  @return Hosts
     */
    const Hosts &hosts() const
    {
        return _hosts;
    }

    /**
     *  Clear the list of nameservers
     */
    void clear()
    {
        // empty the list
        _nameservers.clear();
    }

    /**
     *  Add a nameserver
     *  @param  ip
     */
    void nameserver(const Ip &ip)
    {
        // add to the member in the base class
        _nameservers.emplace_back(ip);
    }

    /**
     *  Total number of nameservers
     *  @return size_t
     */
    size_t nameservers() const
    {
        return _nameservers.size();
    }

    /**
     *  Expose to one of the nameservers
     *  @param  index
     *  @return Ip
     */
    const Ip &nameserver(size_t index) const 
    { 
        return _nameservers[index];
    }

    /**
     *  The total number of searchpaths
     *  @return size_t
     */
    size_t searchpaths() const
    {
        return _searchpaths.size();
    }

    /**
     *  Expose one of the search-paths
     *  @param  index
     *  @return std::string
     */
    const std::string &searchpath(size_t index) const
    { 
        return _searchpaths[index];
    }

    /**
     *  Does a certain hostname exists in /etc/hosts? In that case a NXDOMAIN error should not be given
     *  @param  hostname        hostname to check
     *  @return bool            does it exists in /etc/hosts?
     */
    bool exists(const char *hostname) const
    { 
        // lookup the name
        return _hosts.lookup(hostname) != nullptr;
    }
};

/**
 *  End of namespace
 */
}

