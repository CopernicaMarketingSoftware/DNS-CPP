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
 * 
 *  @todo make this file smaller by moving some methods to the c++ file
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

    /**
     *  Helper method to check if an ipv6 address is actually a mapped ipv4 address
     *  For example: ::ffff:129.168.0.12 is an ipv4 address stored as an ipv6 address
     *  @param  address
     *  @return bool
     */
    static bool isv4mapped(const struct in6_addr &addr)
    {
        // the first 10 bytes should be zero's
        for (size_t i = 0; i < 10; ++i)
        {
            // check if this byte is indeed a zero
            if (addr.s6_addr[i] != 0) return false;
        }
        
        // the next two bytes should be all ones
        return addr.s6_addr[10] == 0xff && addr.s6_addr[11] == 0xff;
    }
    
    
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
    explicit Ip(const char *ip)
    {
        // hints for the call to getaddrinfo()
        struct addrinfo hints;
        
        // fill in the hints
        hints.ai_family = AF_UNSPEC;                    // allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;                // one protocol to get only one address
        hints.ai_flags = AI_NUMERICHOST;                // just parse the address
        hints.ai_protocol = 0;                          // any protocol
        hints.ai_canonname = nullptr;
        hints.ai_addr = nullptr;
        hints.ai_next = nullptr;
        
        // result variable
        struct addrinfo *result = nullptr;
        
        // do the lookup
        auto code = getaddrinfo(ip, nullptr, &hints, &result);

        // invalid IP address supplied
        if (code != 0) throw std::runtime_error("invalid IP address supplied");
        
        // check the address family 
        switch (result->ai_family) {
        case AF_INET:
            // we have a ipv4 address
            _version = 4;
            
            // copy data
            memcpy(&_data.ipv4, &((struct sockaddr_in *)result->ai_addr)->sin_addr, sizeof(struct in_addr));
            break;
            
        case AF_INET6:
            // it could still be an ipv4 address
            if (isv4mapped(((struct sockaddr_in6 *)result->ai_addr)->sin6_addr))
            {
                // turns out to be ipv4 after all
                _version = 4;
                                
                // copy data
                memcpy(&_data.ipv4, ((struct sockaddr_in6 *)result->ai_addr)->sin6_addr.s6_addr + 12, sizeof(struct in_addr));
            }
            else
            {
                // it's indeed ipv6
                _version = 6;
                
                // copy data
                memcpy(&_data.ipv6, ((struct sockaddr_in6 *)result->ai_addr)->sin6_addr.s6_addr, sizeof(struct in6_addr));
            }
            break;
        }

        // throw error when protocol is invalid
        if (_version == 0) throw std::runtime_error("wrong IP protocol version supplied");

        // release memory
        freeaddrinfo(result);
    }


    /**
     *  Constructor based on internal representation of IPv6 address
     *  @param  ip
     */
    explicit Ip(const struct in6_addr *ipv6)
    {
        // are we ipv4 mapped?
        if (isv4mapped(*ipv6))
        {
            // its ipv4 after all
            _version = 4;
            
            // only copy the last four bytes holding the ipv4 address
            memcpy(&_data.ipv4, ipv6->s6_addr + 12, sizeof(struct in_addr));
        }
        else
        {
            // its indeed ipv6
            _version = 6;
            
            // copy the full ipv6 address
            memcpy(&_data.ipv6, ipv6, sizeof(struct in6_addr));
        }
    }

    /**
     *  Constructor based on a socket address
     *  @param  address
     *  @throws Exception
     */
    explicit Ip(const struct sockaddr *addr)
    {
        // check the type of address
        switch (addr->sa_family) {
        case AF_INET:   
            // this is a regular ipv4 address
            _version = 4; 
            
            // copy the data
            memcpy(&_data.ipv4, &((struct sockaddr_in *)(addr))->sin_addr, sizeof(struct in_addr)); 
            break;

        case AF_INET6:
            // looks like an ipv6 address, but it could also be an ipv4 mapped into an ipv6
            if (isv4mapped(((struct sockaddr_in6 *)(addr))->sin6_addr))
            {
                // turns out to be ipv4 after all
                _version = 4;
                
                // copy the data
                memcpy(&_data.ipv4, ((struct sockaddr_in6 *)(addr))->sin6_addr.s6_addr + 12, sizeof(struct in_addr)); 
            }
            else
            {
                // this is indeed an ipv6 address
                _version = 6; 
                
                // copy the data
                memcpy(&_data.ipv6, &((struct sockaddr_in6 *)(addr))->sin6_addr, sizeof(struct in6_addr)); 
            }
            break;
            
        default:    
            throw std::runtime_error("wrong type");
        }
    }
    
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
     *  Convert the address to a string
     *  @return std::string
     */
    std::string str() const
    {
        // check version
        if (_version == 4)
        {
            // construct a buffer
            char buffer[INET_ADDRSTRLEN];

            // convert
            inet_ntop(AF_INET, &_data.ipv4, buffer, INET_ADDRSTRLEN);

            // done
            return std::string(buffer);
        }
        else
        {
            // construct a buffer
            char buffer[INET6_ADDRSTRLEN];

            // convert
            inet_ntop(AF_INET6, &_data.ipv6, buffer, INET6_ADDRSTRLEN);
            
            // done
            return std::string(buffer);
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
    Ip &operator=(const struct in6_addr *ip)
    {
        // the other address could be an ipv4 address mapped in an ipv6 address
        if (isv4mapped(*ip))
        {
            // change version
            _version = 4;
            
            // copy just the ipv4 data
            memcpy(&_data.ipv4, ip->s6_addr + 12, sizeof(struct in_addr));
        }
        else
        {
            // change version
            _version = 6;

            // copy data
            memcpy(&_data.ipv6, ip, sizeof(struct in6_addr));
        }
        
        // allow chaining
        return *this;
    }

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
    friend std::ostream &operator<<(std::ostream &stream, const Ip &ip)
    {
        // check version
        if (ip._version == 4)
        {
            // construct a buffer
            char buffer[INET_ADDRSTRLEN];

            // convert
            inet_ntop(AF_INET, &ip._data.ipv4, buffer, INET_ADDRSTRLEN);

            // write buffer
            return stream << buffer;
        }
        else
        {
            // construct a buffer
            char buffer[INET6_ADDRSTRLEN];

            // convert
            inet_ntop(AF_INET6, &ip._data.ipv6, buffer, INET6_ADDRSTRLEN);
            
            // write buffer
            return stream << buffer;
        }
    }
};
    
/**
 *  End of namespace
 */
}
