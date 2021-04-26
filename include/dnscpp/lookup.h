/**
 *  Lookup.h
 * 
 *  This is the base class for all lookup implementations. It is only
 *  used internally, user space code does not interact with it.
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
#include "operation.h"
#include "intrusivequeueitem.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
//class Handler;

/**
 *  Class definition
 */
class Lookup : public Operation, public IntrusiveQueueItem<Lookup>
{
protected:
    /**
     *  Constructor
     *  @param  handler     user space handler
     *  @param  op          the type of operation (normally a regular query)
     *  @param  dname       the domain to lookup
     *  @param  type        record type to look up
     *  @param  bits        extra bits to be included in the query
     *  @param  data        optional data (only for type = ns_o_notify)
     *  @throws std::runtime_error
     */
    Lookup(Handler *handler, int op, const char *dname, int type, const Bits &bits, const unsigned char *data = nullptr) :
        Operation(handler, op, dname, type, bits, data) {}

public:
    /**
     *  Destructor
     */
    virtual ~Lookup() = default;
    
    /**
     *  Is this lookup still scheduled: meaning that no requests has been sent yet
     *  @return bool
     */
    virtual bool scheduled() const = 0;
    
    /**
     *  Is this lookup already finished: meaning that a result has been reported back to userspace
     *  @return bool
     */
    virtual bool finished() const = 0;
    
    /**
     *  Is this lookup exhausted: meaning that it has sent its max number of requests, but still
     *  has not received an appropriate answer, and is now waiting for its final timer to finish
     *  @return bool
     */
    virtual bool exhausted() const = 0;

    /**
     *  Is this lookup expired: meaning did the lookup took too long
     *  @param  double      current time
     *  @return bool        true if it timed out
     */
    virtual bool expired(double now) const noexcept = 0;

    /**
     *  How long should we wait until the next runtime?
     *  @param  double      current time
     *  @return double      delay in seconds
     */
    virtual double delay(double now) const = 0;
    
    /**
     *  Execute this lookup.
     * 
     *  Watch out: this method should NOT be called when a lookup is already finished!!
     *  or when it has no more attempts left
     * 
     *  @param  now         current time
     *  @return bool        whether the execution succeeded
     */
    virtual bool execute(double now) = 0;

    /**
     *  This lookup is done. Either it exhausted all of its attempts or it has a result. Invoke the callback handler
     */
    virtual void finalize() = 0;
};
    
/**
 *  End of namespace
 */
}
