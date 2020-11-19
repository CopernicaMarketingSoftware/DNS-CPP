/**
 *  Core.h
 * 
 *  The core is the private base class of the dns context and is used
 *  internally by the dns-cpp library. This class is not accessible
 *  in user space. The public methods in this class can therefore not
 *  be called from user space, but they are used internally.
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
#include "nameserver.h"
#include "resolvconf.h"
#include "hosts.h"
#include <list>

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Forward declarations
 */
class Loop;

/**
 *  Class definition
 */
class Core
{
protected:
    /**
     *  Pointer to the event loop supplied by user space
     *  @var Loop
     */
    Loop *_loop;

    /**
     *  The IP addresses of the servers that can be accessed
     *  @var std::list<Nameserver>
     */
    std::list<Nameserver> _nameservers;
    
    /**
     *  The contents of the /etc/hosts file
     *  @var Hosts
     */
    Hosts _hosts;
    
    /**
     *  Size of the send and receive buffer. If set to zero, default
     *  will be kept. This is limited by the system maximum (wmem_max and rmem_max)
     *  @var size_t
     */
    int32_t _buffersize = 0;

    /**
     *  Maximum number of sockets that may be opened per nameserver. By default, this is set to 32.
     *  @var size_t
     */
    size_t _sockets = 32;

    /**
     *  Maximum number of open requests per socket on average.
     *  @var size_t
     */
    size_t _socketrequests = 64;

    /**
     *  Max time that a request may last in seconds
     *  @var double
     */
    double _expire = 60.0;
    
    /**
     *  During each attempt all nameservers are contacted at the same time,
     *  with a small interval between them - this is that interval
     *  @var double
     */
    double _spread = 0.1;
    
    /**
     *  Interval before a datagram is repeated to the same server, in seconds
     *  @var double
     */
    double _interval = 2.0;
    
    /**
     *  Is DNSSEC-querying enabled?
     *  @var bool
     */
    bool _dnssec = false;

protected:
    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     *  @param  defaults    should defaults from resolv.conf and /etc/hosts be loaded?
     *  @throws std::runtime_error
     */
    Core(Loop *loop, bool defaults);

    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     *  @param  settings    settings from the resolv.conf file
     * 
     *  @deprecated
     */
    Core(Loop *loop, const ResolvConf &settings) : _loop(loop) 
    {
        // construct the nameservers
        for (size_t i = 0; i < settings.nameservers(); ++i)
        {
            // construct a nameserver
            _nameservers.emplace_back(this, settings.nameserver(i));
        }
    }
    
    /**
     *  Destructor
     */
    virtual ~Core() = default;

public:
    /**
     *  No copying
     *  @param  that
     */
    Core(const Core &that) = delete;
    
    /**
     *  Expose the event loop
     *  @return Loop
     */
    Loop *loop() { return _loop; }

    /**
     *  The send and receive buffer size 
     *  @return int32_t
     */
    int32_t buffersize() const { return _buffersize; }

    /**
     *  Maximum number of open sockets per nameserver.
     *  @return size_t
     */
    size_t sockets() const { return _sockets; }

    /**
     *  Maximum number of open requests per socket before opening a new socket.
     *  @return size_t
     */
    size_t socketrequests() const { return _socketrequests; }    
    
    /**
     *  The period between sending a new datagram to the same nameserver in seconds
     *  @return double
     */
    double interval() const { return _interval; }
    
    /**
     *  Interval between sending datagrams to different servers
     *  @return double
     */
    double spread() const { return _spread; }
    
    /**
     *  The expire time for a request in seconds
     *  @return double
     */
    double expire() const { return _expire; }
    
    /**
     *  Should we also query for dnssec properties?
     *  @return bool
     */
    bool dnssec() const { return _dnssec; }
    
    /**
     *  Does a certain hostname exists in /etc/hosts? In that case a NXDOMAIN error should not be given
     *  @param  hostname        hostname to check
     *  @return bool            does it exists in /etc/hosts?
     */
    bool exists(const char *hostname) const { return _hosts.lookup(hostname) != nullptr; }
    
    /**
     *  Expose the nameservers
     *  @return std::list<Nameserver>
     */
    std::list<Nameserver> &nameservers()
    {
        // expose the member
        return _nameservers;
    }
};

/**
 *  End of namespace
 */
}
