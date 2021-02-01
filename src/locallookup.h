/**
 *  LocalLookup.h
 * 
 *  Class that implements the lookup in the local /etc/hosts file
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */
 
/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "../include/dnscpp/request.h"
#include "../include/dnscpp/question.h"
#include "../include/dnscpp/reverse.h"

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Class definition
 */
class LocalLookup : public Lookup
{
private:
    /**
     *  Reference to the /etc/hosts file
     *  @var Hosts
     */
    const Hosts &_hosts;
    
    /**
     *  Is the operation ready?
     *  @var bool
     */
    bool _ready = false;

    /**
     *  Method that is called when it is time to process this lookup
     *  @param  now     current time
     *  @return bool    should it be rescheduled?
     */
    virtual bool execute(double now) override
    {
        // do nothing if ready
        if (_ready) return false;
        
        // pass to the hosts
        _hosts.notify(Request(this), _handler, this);
        
        // remember that the operation is ready
        _ready = true;
        
        // no need to reschedule
        return false;
    }

    /**
     *  How long should we wait until the next runtime?
     *  @param  now         current time
     *  @return double      delay in seconds
     */
    virtual double delay(double now) const override
    {
        // should run right away
        return 0.0;
    }

    /**
     *  How many (not yet successful) attempts have already been made / requests sent?
     *  @return size_t      number of attempts
     */
    virtual size_t attempts() const override
    {
        // local lookups success at the first attempt
        return 0;
    }
    
public:
    /**
     *  Constructor
     *  To keep the behavior of lookups consistent with the behavior of remote lookups, we set
     *  a timer so that userspace will be informed in a later tick of the event loop
     *  @param  hosts
     *  @param  domain
     *  @param  type
     *  @param  handler
     */
    LocalLookup(const Hosts &hosts, const char *domain, int type, Handler *handler) : 
        Lookup(handler, ns_o_query, domain, type, false), _hosts(hosts) {}

    /**
     *  Constructor
     *  This is a utility constructor for reverse lookups
     *  @param  hosts
     *  @param  ip
     *  @param  handler
     */
    LocalLookup(const Hosts &hosts, const Ip &ip, Handler *handler) : LocalLookup(hosts, Reverse(ip), TYPE_PTR, handler) {}

    /**
     *  Destructor
     *  This is a self-destructing class
     */
    virtual ~LocalLookup()
    {
        // if the operation is destructed while it was still running, it means that the
        // operation was prematurely cancelled from user-space, let the handler know
        // @todo check if this is correct  / also implement the cancel() method
        if (!_ready) _handler->onCancelled(this);
    }
};
    
/**
 *  End of namespace
 */
}


