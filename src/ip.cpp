/**
 *  IP.cpp
 * 
 *  Implementation file for the IP class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/printable.h"
#include "../include/dnscpp/record.h"
#include "../include/dnscpp/type.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/a.h"
#include "../include/dnscpp/aaaa.h"

/**
 *  Begin of namespace
 */
namespace DNS {

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
    
/**
 *  Helper function to extract an IP from a record (only works for A and AAAA)
 *  @param  record
 *  @return Ip
 *  @throws std::runtime_error
 */
static Ip extract(const Record &record)
{
    // check the record
    switch (record.type()) {
    case TYPE_A:    return A(record).ip();
    case TYPE_AAAA: return AAAA(record).ip();
    default:        throw std::runtime_error("Ips can only be extracted from A and AAAA records");
    }
}
    
/**
 *  Constructor based on a string representation
 *  @param  ip          The IP address to parse
 *  @throws std::runtime_error
 */
Ip::Ip(const char *ip)
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

    // release memory
    freeaddrinfo(result);

    // throw error when protocol is invalid
    if (_version == 0) throw std::runtime_error("wrong IP protocol version supplied");
}

/**
 *  Constructor based on internal representation of IPv6 address
 *  @param  ip
 */
Ip::Ip(const struct in6_addr *ipv6)
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
Ip::Ip(const struct sockaddr *addr)
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
 *  Methods to extract IP addresses from records
 *  @param  record
 */
Ip::Ip(const A &record) : Ip(record.ip()) {}
Ip::Ip(const AAAA &record) : Ip(record.ip()) {}
Ip::Ip(const Record &record) : Ip(extract(record)) {}

/**
 *  Assignment operators
 *  @param  ip
 *  @return IpAddress
 */
Ip &Ip::operator=(const struct in6_addr *ip)
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
 *  Inverse operator
 *  @return Ip
 */
Ip Ip::operator~() const
{
    // implementation depends on ip version
    if (_version == 6)
    {
        // get the address
        auto address = _data.ipv6;
        
        // iterate over the entire address
        for (unsigned i = 0; i < 16; ++i) address.s6_addr[i] = ~address.s6_addr[i];
        
        // construct a new ip
        return DNS::Ip(address);
    }
    else
    {
        // get the address
        auto address = _data.ipv4;
        
        // ipv4 can use a single cpu operation
        address.s_addr = ~address.s_addr;

        // construct a new ip
        return DNS::Ip(address);
    }
}

/**
 *  Bitwise assignment OR operator (x |= y)
 *  @param  that        The other IP
 *  @return Ip
 */
Ip &Ip::operator|=(const Ip &that)
{
    // skip for version-mismatch
    if (_version != that._version) return *this;
    
    // implementation depends on ip version
    if (_version == 6)
    {
        // get the two addresses
        auto *this_address = _data.ipv6.s6_addr;
        auto *that_address = that._data.ipv6.s6_addr;
        
        // iterate over the entire address
        for (unsigned i = 0; i < 16; ++i) this_address[i] |= that_address[i];
    }
    else
    {
        // ipv4 can use a single cpu operation
        _data.ipv4.s_addr |= that._data.ipv4.s_addr;
    }
    
    // done
    return *this;
}

/**
 *  Bitwise assignment AND operator (x &= y)
 *  This only works if both objects have the same version (both ipv4 or both ipv6),
 *  if the other object is of a different version, the behavior is undefined.
 *  @param  that        The other IP
 *  @return Ip
 */
Ip &Ip::operator&=(const Ip &that)
{
    // skip for version-mismatch
    if (_version != that._version) return *this;
    
    // implementation depends on ip version
    if (_version == 6)
    {
        // get the two addresses
        auto *this_address = _data.ipv6.s6_addr;
        auto *that_address = that._data.ipv6.s6_addr;
        
        // iterate over the entire address
        for (unsigned i = 0; i < 16; ++i) this_address[i] &= that_address[i];
    }
    else
    {
        // ipv4 can use a single cpu operation
        _data.ipv4.s_addr &= that._data.ipv4.s_addr;
    }
    
    // done
    return *this;
}


/**
 *  Write to a stream
 *  @param  stream
 *  @param  ip
 *  @return std::ostream
 */
std::ostream &operator<<(std::ostream &stream, const Ip &ip)
{
    // convert to printable representation, and print that
    return stream << Printable(ip);
}

    
/**
 *  End of namespace
 */
}
