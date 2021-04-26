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
     *  Execute this lookup.
     *
     *  Watch out: this method should NOT be called when a lookup is already finished!!
     *  or when it has no more attempts left
     *
     *  @param  now         current time
     *  @return bool        whether the execution succeeded
     */
    virtual bool execute(double now) override
    {
        // always fail
        return false;
    }

    /**
     *  Is this lookup expired: meaning did the lookup took too long
     *  @param  double      current time
     *  @return bool        true if it timed out
     */
    virtual bool expired(double now) const noexcept override
    {
        // we're always expired
        return true;
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
     *  Is this lookup still scheduled: meaning that no requests has been sent yet
     *  @return bool
     */
    virtual bool scheduled() const override
    {
        // because the handler is reset on completion, we can use it to see if the operation is still scheduled
        return _handler != nullptr;
    }
    
    /**
     *  Is this lookup already finished: meaning that a result has been reported back to userspace
     *  @return bool
     */
    virtual bool finished() const override
    {
        // handler is reset on completion
        return _handler == nullptr;
    }
    
    /**
     *  Is this lookup exhausted: meaning that it has sent its max number of requests, but still
     *  has not received an appropriate answer, and is now waiting for its final timer to finish
     *  @return bool
     */
    virtual bool exhausted() const override
    {
        // put me in the ready queue immediately
        return true;
    }

    /**
     *  Invoke callback handler
     */
    virtual void finalize() override
    {
        // do nothing if ready
        assert(!finished());

        // remember the handler
        auto *handler = _handler;

        // get rid of the handler to avoid that the result is reported
        _handler = nullptr;

        // pass to the hosts (this will trigger an immediate call to the handler)
        _hosts.notify(Request(this), handler, this);
    }
    
    /**
     *  Cancel the lookup
     */
    virtual void cancel() override
    {
        // if already reported back to user-space
        if (_handler == nullptr) return;
        
        // remember the handler
        auto *handler = _handler;
        
        // get rid of the handler to avoid that the result is reported
        _handler = nullptr;
        
        // the last instruction is to report it back to user-space
        handler->onCancelled(this);
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
        if (_handler != nullptr) _handler->onCancelled(this);
    }
};
    
/**
 *  End of namespace
 */
}


