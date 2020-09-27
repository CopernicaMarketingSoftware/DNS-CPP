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
     *  Max time that a request may last in seconds
     *  @var double
     */
    double _expire = 60.0;
    
    /**
     *  Interval between attempts, after this period a new dgram is sent
     *  @var double
     */
    double _interval = 2.0;
    
    /**
     *  Is DNSSEC-querying enabled?
     *  @var bool
     */
    bool _dnssec = true;

protected:
    /**
     *  Protected constructor, only the derived class may construct it
     *  @param  loop        your event loop
     */
    Core(Loop *loop) : _loop(loop) {}
    
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
     *  The period between sending a new datagram in seconds
     *  @return double
     */
    double interval() const { return _interval; }
    
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
