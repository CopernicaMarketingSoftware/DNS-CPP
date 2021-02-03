/**
 *  Received.h
 * 
 *  Because the speed at which UDP packets come in can be higher than
 *  the speed at which they can be processed by user-space, the DNS-CPP
 *  library first reads out the socket, buffers the responses, and postpones
 *  parsing those responses until the event loop is idle.
 * 
 *  This class implements one of those received, unparsed, messages
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "ip.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Core;

/**
 *  Class definition
 */
class Received
{
private:
    /**
     *  IP address from which the message is received
     *  @var Ip
     */
    Ip _ip;
    
    /**
     *  The received buffer
     *  @var std::string
     */
    std::string _buffer;

public:
    /**
     *  Constructor
     *  @param  ip
     *  @param  buffer
     *  @param  size
     */
    Received(const struct sockaddr *ip, const unsigned char *buffer, size_t size) :
        _ip(ip), _buffer((const char *)buffer, size) {}
    
    /**
     *  No copying because copying buffers is too expensive
     *  @param  that
     */
    Received(const Received &that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~Received() = default;
    
    /**
     *  The IP from which this message was received
     *  @return Ip
     */
    const Ip &ip() const { return _ip; }
    
    /**
     *  Expose the buffer
     *  @return const unsigned char *
     */
    const unsigned char *data() const { return (unsigned char *)_buffer.data(); }
    
    /**
     *  Size of the buffer
     *  @return size_t
     */
    size_t size() const { return _buffer.size(); }
};

/**
 *  End of namespace
 */
}

