/**
 *  Printable.h
 * 
 *  Helper class to make a printable / string representation of an IP
 *  address.
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
class Printable
{
private:
    /**
     *  The internal buffer
     *  @var char[]
     */
    char _buffer[INET6_ADDRSTRLEN];

public:
    /**
     *  Constructor
     *  @param  ip
     *  @throws std::runtime_error
     */
    Printable(const Ip &ip)
    {
        // check version
        if (ip.version() == 4)
        {
            // convert
            if (inet_ntop(AF_INET, (const struct in_addr *)ip, _buffer, INET_ADDRSTRLEN) != nullptr) return;
        }
        else
        {
            // convert
            if (inet_ntop(AF_INET6, (const struct in6_addr *)ip, _buffer, INET6_ADDRSTRLEN) != nullptr) return;
        }
        
        // report an error
        throw std::runtime_error(strerror(errno));
    }

    /**
     *  Destructor
     */
    virtual ~Printable() = default;
    
    /**
     *  Expose the IP address
     *  @return const char *
     */
    const char *data() const { return _buffer; }
    size_t size() const { return strlen(_buffer); }
    
    /**
     *  Casting operator so that the object can be used as a const-char-pointer
     *  @return const char *
     */
    operator const char * () const { return _buffer; }
    
    /**
     *  Write to a stream
     *  @param  stream
     *  @param  ip
     *  @return std::ostream
     */
    friend std::ostream &operator<<(std::ostream &stream, const Printable &ip)
    {
        // pass to the stream
        return stream.write(ip.data(), ip.size());
    }
};
    
/**
 *  End of namespace
 */
}
