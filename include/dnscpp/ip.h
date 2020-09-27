/**
 *  IP.h
 * 
 *  Class that represents one IP address
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
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ostream>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Ip
{
private:
    /**
     *  IP version (can be 4 or 6)
     *  @var uint8_t
     */
    uint8_t _version = 0;
    
    /**
     *  The buffer holding the IP address
     *  @var union
     */
    union {
        struct in_addr  ipv4;
        struct in6_addr ipv6;
    } _data;

public:
    /**
     *  Constructor for the default 0.0.0.0 address
     *  @param  version
     *  @throws std::runtime_error
     */
    explicit Ip(size_t version = 4) : _version(version)
    {
        // all should be zero
        memset(&_data, 0, sizeof(_data));
        
        // version must be right
        if (_version == 4 || _version == 6) return;
        
        // report error
        throw std::runtime_error("invalid ip version");
    }

    /**
     *  Constructor based on internal representation of IPv4 address
     *  @param  ip
     */
    explicit Ip(const struct in_addr *ipv4) : _version(4)
    {
        // copy data
        memcpy(&_data.ipv4, ipv4, sizeof(struct in_addr));
    }

    /**
     *  Constructor based on a string representation
     *  @param  ip          The IP address to parse
     *  @throws std::runtime_error
     */
    explicit Ip(const char *ip);

    /**
     *  Constructor based on internal representation of IPv6 address
     *  @param  ip
     */
    explicit Ip(const struct in6_addr *ipv6);

    /**
     *  Constructor based on a socket address
     *  @param  address
     *  @throws Exception
     */
    explicit Ip(const struct sockaddr *addr);
    
    /**
     *  Copy constructor
     *  @param  that
     */
    Ip(const Ip &that) : 
        _version(that._version),
        _data(that._data) {}

    /**
     *  Various other constructors that simply pass on their call to one of the other constructor
     *  @param  ip          The IP in a specific format
     *  @throws Exception
     */
    explicit Ip(const struct in_addr &ip) : Ip(&ip) {}
    explicit Ip(const struct in6_addr &ip) : Ip(&ip) {}
    explicit Ip(const struct sockaddr &ip) : Ip(&ip) {}
    explicit Ip(const struct sockaddr_in &ip) : Ip(ip.sin_addr) {}
    explicit Ip(const struct sockaddr_in *ip) : Ip(ip->sin_addr) {}
    explicit Ip(const struct sockaddr_in6 &ip) : Ip(ip.sin6_addr) {}
    explicit Ip(const struct sockaddr_in6 *ip) : Ip(ip->sin6_addr) {}

    /**
     *  Destructor
     */
    virtual ~Ip() = default;

    /**
     *  Version: IPv4 or IPv6?
     *  @return unsigned
     */
    unsigned version() const
    {
        // expose member
        return _version;
    }
    
    /**
     *  Access to the raw binary data, in host-byte-order
     *  @return const char *
     */
    const char *data() const
    {
        // expose the data
        return (const char *)&_data;
    }
    
    /**
     *  Size of the address (number of bytes that it occupies base on version)
     *  @return size_t
     */
    size_t size() const
    {
        switch (_version) {
        case 4:     return sizeof(_data.ipv4);
        case 6:     return sizeof(_data.ipv6);
        default:    return 0;
        }
    }
    
    /**
     *  Compare addresses
     *  @param  address
     */
    bool operator==(const Ip &ip) const
    {
        // identical objects
        if (this == &ip) return true;
    
        // ip versions should be identical
        if (_version != ip._version) return false;
        
        // compare data
        switch (_version) {
        case 4: return memcmp(&_data.ipv4, &ip._data.ipv4, sizeof(struct in_addr)) == 0;
        case 6: return memcmp(&_data.ipv6, &ip._data.ipv6, sizeof(struct in6_addr)) == 0;
        default:return true;
        }
    }

    /**
     *  Compare two IP addresses
     *  @param  ip      The address to compare
     *  @return bool
     */
    bool operator!=(const Ip &address) const
    {
        return !operator==(address);
    }

    /**
     *  Compare addresses
     *  @param  address
     *  @return bool
     */
    bool operator<(const Ip &ip) const
    {
        // identical objects
        if (this == &ip) return false;
    
        // ip versions should be identical
        if (_version != ip._version) return _version < ip._version;
        
        // compare data
        switch (_version) {
        case 4: return memcmp(&_data.ipv4, &ip._data.ipv4, sizeof(struct in_addr)) < 0;
        case 6: return memcmp(&_data.ipv6, &ip._data.ipv6, sizeof(struct in6_addr)) < 0;
        default:return false;
        }
    }

    /**
     *  Compare addresses
     *  @param  address
     *  @return bool
     */
    bool operator>(const Ip &ip) const
    {
        // identical objects
        if (this == &ip) return false;
    
        // ip versions should be identical
        if (_version != ip._version) return _version > ip._version;
        
        // compare data
        switch (_version) {
        case 4: return memcmp(&_data.ipv4, &ip._data.ipv4, sizeof(struct in_addr)) > 0;
        case 6: return memcmp(&_data.ipv6, &ip._data.ipv6, sizeof(struct in6_addr)) > 0;
        default:return false;
        }
    }
    
    /**
     *  Cast to a "struct in_addr"
     *  @return struct in_addr
     */
    operator const struct in_addr& () const
    {
        return _data.ipv4;
    }

    /**
     *  Cast to a "struct in6_addr"
     *  @return struct in_addr
     */
    operator const struct in6_addr& () const
    {
        return _data.ipv6;
    }

    /**
     *  Cast to a "struct in_addr *"
     *  @return struct in_addr
     */
    operator const struct in_addr* () const
    {
        return &_data.ipv4;
    }

    /**
     *  Cast to a "struct in6_addr*"
     *  @return struct in_addr
     */
    operator const struct in6_addr* () const
    {
        return &_data.ipv6;
    }
    
    /**
     *  Assignment operators
     *  @param  ip
     *  @return IpAddress
     */
    Ip &operator=(const struct in_addr *ip)
    {
        // change version
        _version = 4;
        
        // copy data
        memcpy(&_data.ipv4, ip, sizeof(struct in_addr));
        
        // allow chaining
        return *this;
    }

    /**
     *  Assignment operators
     *  @param  ip
     *  @return IpAddress
     */
    Ip &operator=(const struct in_addr &ip)
    {
        // pass on
        return operator=(&ip);
    }

    /**
     *  Assignment operators
     *  @param  ip
     *  @return IpAddress
     */
    Ip &operator=(const struct in6_addr *ip);

    /**
     *  Assignment operators
     *  @param  ip
     *  @return IpAddress
     */
    Ip &operator=(const struct in6_addr &ip)
    {
        // pass on
        return operator=(&ip);
    }
    
    /**
     *  Is this the INADDR_ANY address?
     *  @return bool
     */
    bool any() const
    {
        // for ipv4 addresses
        if (_version == 4) return _data.ipv4.s_addr == INADDR_ANY;
        
        // for ipv6 addresses
        if (_version == 6) return memcmp(&_data.ipv6, &in6addr_any, sizeof(struct in6_addr)) == 0;
        
        // failure
        return false;
    }
    
    /**
     *  Is this a valid IP address (0.0.0.0 is invalid, all others are valid)
     *  @return bool
     */
    operator bool () const
    {
        return !any();
    }
    
    /**
     *  Is this the ANY address?
     *  @return bool
     */
    bool operator! () const
    {
        return any();
    }
    
    /**
     *  Write to a stream
     *  @param  stream
     *  @param  ip
     *  @return std::ostream
     */
    friend std::ostream &operator<<(std::ostream &stream, const Ip &ip);
};
    
/**
 *  End of namespace
 */
}
