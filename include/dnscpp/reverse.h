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
     *  Destructor
     */
    virtual ~Reverse() = default;

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

