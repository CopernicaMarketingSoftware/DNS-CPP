/**
 *  LocalLookup.h
 * 
 *  Class that implements the lookup in the local /etc/hosts file
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
class LocalLookup : public Operation, private Timer
{
private:
    /**
     *  Reference the event loop
     *  @var Loop
     */
    Loop *_loop;
    
    /**
     *  Reference to the /etc/hosts file
     *  @var Hosts
     */
    const Hosts &_hosts;
    
    /**
     *  The running timer
     *  @var void*
     */
    void *_timer = nullptr;
    

    /**
     *  Method that is called when the timer expires
     */
    virtual void expire() override
    {
        // forget about the timer
        _loop->cancel(_timer, this); _timer = nullptr;
        
        // pass to the hosts
        _hosts.notify(Request(this), _handler, this);
        
        // we can now self-destruct, userspace has been informed
        delete this;
    }
    
    /**
     *  Private destructor
     *  This is a self-destructing class
     */
    virtual ~LocalLookup()
    {
        // do nothing if timer is already dead
        if (_timer == nullptr) return;
        
        // stop the timer
        _loop->cancel(_timer, this);
        
        // if the operation is destructed while the timer was still running, it means that the
        // operation was prematurely cancelled from user-space, let the handler know
        _handler->onCancelled(this);
    }

public:
    /**
     *  Constructor
     *  To keep the behavior of lookups consistent with the behavior of remote lookups, we set
     *  a timer so that userspace will be informed in a later tick of the event loop
     *  @param  loop
     *  @param  hosts
     *  @param  domain
     *  @param  type
     *  @param  handler
     */
    LocalLookup(Loop *loop, const Hosts &hosts, const char *domain, int type, Handler *handler) : 
        Operation(handler, ns_o_query, domain, type, false), 
        _loop(loop), 
        _hosts(hosts),
        _timer(_loop->timer(0.0, this)) {}

    /**
     *  Constructor
     *  This is a utility constructor for reverse lookups
     *  @param  loop
     *  @param  hosts
     *  @param  ip
     *  @param  handler
     */
    LocalLookup(Loop *loop, const Hosts &hosts, const Ip &ip, Handler *handler) : LocalLookup(loop, hosts, Reverse(ip), TYPE_PTR, handler) {}
};
    
/**
 *  End of namespace
 */
}


