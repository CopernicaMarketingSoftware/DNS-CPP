/**
 *  LocalLookup.h
 * 
 *  Class that implements the lookup in the local /etc/hosts file
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2025 Copernica BV
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
     *  Execute the lookup. Returns true when a user-space call was made, and false when further
     *  processing is required.
     *  @param  now         current time
     *  @return bool        was there a call back to userspace?
     */
    virtual bool execute(double now) override
    {
        // do nothing if ready
        assert(!finished());

        // remember the handler
        auto *handler = _handler;
        
        // get rid of the handler to avoid that the result is reported
        _handler = nullptr;

        // pass to the hosts (this will trigger an immediate call to the handler)
        _config->hosts().notify(Request(this), handler, this);
        
        // done
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
     *  Is this lookup still scheduled: meaning that no requests have been sent yet
     *  @return bool
     */
    virtual bool scheduled() const override
    {
        // we return false here, because the very first call to execute() will immediately trigger a 
        // call to user-space, AS IF we already sent out one or more requests
        return false;
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
        // because the local lookup does not have to send out requests, it is by definition exhausted
        return true;
    }

    /**
     *  Cancel the operation
     */
    virtual void cancel() override
    {
        // if already reported back to user-space
        if (_handler == nullptr) return;
        
        // notify the core object so that it can schedule more things
        // NOTE that this is not so elegant, as it is not the responsibility of the Lookup class 
        // to keep the bookkeeping of the Core class correct
        _core->cancel(this);
        
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
     *  @param  core
     *  @param  config
     *  @param  domain
     *  @param  type
     *  @param  handler
     */
    LocalLookup(Core *core, const std::shared_ptr<Config> &config, const char *domain, int type, Handler *handler) :
        Lookup(core, config, handler, ns_o_query, domain, type, Bits{}) {}

    /**
     *  Constructor
     *  This is a utility constructor for reverse lookups
     *  @param  core
     *  @param  config
     *  @param  ip
     *  @param  handler
     */
    LocalLookup(Core *core, const std::shared_ptr<Config> &config, const Ip &ip, Handler *handler) : LocalLookup(core, config, Reverse(ip), TYPE_PTR, handler) {}

    /**
     *  Destructor
     *  This is a self-destructing class
     */
    virtual ~LocalLookup() = default;
};
    
/**
 *  End of namespace
 */
}


