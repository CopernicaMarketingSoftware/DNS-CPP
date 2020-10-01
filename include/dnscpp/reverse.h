/**
 *  Reverse.h
 * 
 *  Class that can be used to turn an IP address into "reverse"
 *  notation, which is the "in-addr.arpa" notation that is needed
 *  for a PTR record lookup.
 * 
 *  It is internally used by the library when you do a query for a
 *  PTR record given an IP address
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
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
class Reverse
{
private:
private:
    /**
     *  Buffer to which data is written
     *  @var char[]
     */
    char _buffer[128];


    /**
     *  Scan the buffer as ipv4 address
     *  @return Ip
     */
    Ip scanipv4() const
    {
        // the integers that will be filled
        unsigned int bytes[4];
        
        // scan the input-string
        sscanf(_buffer, "%u.%u.%u.%u", &bytes[3], &bytes[2], &bytes[1], &bytes[0]);

        // the address that we will fill
        struct in_addr address;
        
        // fill the bytes
        address.s_addr = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
        
        // expose the address
        return Ip(address);
    }

    /**
     *  Scan the buffer as ipv6 address
     *  @return Ip
     */
    Ip scanipv6() const
    {
        // the integers that will be filled
        unsigned int bytes[32];
        
        // scan the input-string
        sscanf(_buffer, "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x", 
            &bytes[31], &bytes[30], &bytes[29], &bytes[28], &bytes[27], &bytes[26], &bytes[25], &bytes[24],
            &bytes[23], &bytes[22], &bytes[21], &bytes[20], &bytes[19], &bytes[18], &bytes[17], &bytes[16],
            &bytes[15], &bytes[14], &bytes[13], &bytes[12], &bytes[11], &bytes[10], &bytes[ 9], &bytes[ 8],
            &bytes[ 7], &bytes[ 6], &bytes[ 5], &bytes[ 4], &bytes[ 3], &bytes[ 2], &bytes[ 1], &bytes[ 0]
        );

        // the address that we will fill
        struct in6_addr address;
        
        // fill the bytes
        for (size_t i = 0; i < 16; ++i) address.s6_addr[i] = (bytes[i * 2] << 4) | bytes[i * 2 + 1];
        
        // expose the address
        return Ip(address);
    }
    
public:
    /**
     *  Constructor
     *  @param  ip
     */
    Reverse(const Ip &ip)
    {
        // is this an ipv4 address?
        if (ip.version() == 4)
        {
            // get the internal structure
            const struct in_addr &addr = ip;
            
            // cast the address to a char buffer
            auto *bytes = (unsigned char *)&addr.s_addr;

            // fill buffer
            sprintf(_buffer, "%u.%u.%u.%u.in-addr.arpa", bytes[3], bytes[2], bytes[1], bytes[0]);
        }
        else
        {
            // get the internal structure
            const struct in6_addr &addr = ip;
            
            // pointer to the bytes
            auto *bytes = (unsigned char *)&addr.s6_addr;

            // write the parts in reverse order
            sprintf(_buffer, "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.ip6.arpa",
                bytes[15]&0xf, bytes[15] >> 4, bytes[14]&0xf, bytes[14] >> 4,
                bytes[13]&0xf, bytes[13] >> 4, bytes[12]&0xf, bytes[12] >> 4,
                bytes[11]&0xf, bytes[11] >> 4, bytes[10]&0xf, bytes[10] >> 4,
                bytes[9]&0xf,  bytes[9]  >> 4, bytes[8]&0xf,  bytes[8]  >> 4,
                bytes[7]&0xf,  bytes[7]  >> 4, bytes[6]&0xf,  bytes[6]  >> 4,
                bytes[5]&0xf,  bytes[5]  >> 4, bytes[4]&0xf,  bytes[4]  >> 4,
                bytes[3]&0xf,  bytes[3]  >> 4, bytes[2]&0xf,  bytes[2]  >> 4,
                bytes[1]&0xf,  bytes[1]  >> 4, bytes[0]&0xf,  bytes[0]  >> 4
            );
        }
    }
    
    /**
     *  Constructor to parse an existing reverse-address
     *  @param  address
     *  @throws std::runtime_error
     */
    Reverse(const char *address)
    {
        // address should not be too long
        if (strlen(address) >= sizeof(_buffer)) throw std::runtime_error("address is too long to contain an IPv4 address");
        
        // copy the address
        strcpy(_buffer, address);
    
        // check version, it should be valid now
        if (!version()) throw std::runtime_error("address is not in reverse notation");
    }
    
    /**
     *  Destructor
     */
    virtual ~Reverse() = default;

    /**
     *  The IP version
     *  @return int
     */
    int version() const
    {
        // we need the size a number of times
        size_t size = this->size();
        
        // does it end with "in-addr.arpa"? then we have an ipv4 address
        if (size > 13 && strcasecmp(_buffer + size - 13, ".in-addr.arpa") == 0) return 4;

        // does it end with "ip6.arpa"? then it's a ipv6 address
        if (size > 9 && strcasecmp(_buffer + size - 9, ".ip6.arpa") == 0) return 6;
        
        // this should not be possible
        return 0;
    }
    
    /**
     *  Restore the original IP address
     *  @return Ip
     */
    Ip ip() const
    {
        // check the version
        return version() == 4 ? scanipv4() : scanipv6();
    }

    /**
     *  Expose the IP address
     *  @return const char *
     */
    const char *data() const { return _buffer; }
    size_t size() const { return strlen(_buffer); }
    
    /**
     *  Cast to a const char *
     *  @return const char *
     */
    operator const char * () const { return _buffer; }

    /**
     *  Write to a stream
     *  @param  stream
     *  @param  ip
     *  @return std::ostream
     */
    friend std::ostream &operator<<(std::ostream &stream, const Reverse &ip)
    {
        // pass to the stream
        return stream.write(ip.data(), ip.size());
    }
};

/**
 *  End of namespace
 */
}

