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

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Handler;
class Core;

/**
 *  Class definition
 */
class Lookup : public Operation
{
protected:
    /**
     *  The core object
     *  @var Core
     */
    Core *_core;

    /**
     *  The query that we're going to send
     *  @var Query
     */
    const Query _query;

    /**
     *  Constructor
     *  @param  core        the core object
     *  @param  handler     user space handler
     *  @param  op          the type of operation (normally a regular query)
     *  @param  dname       the domain to lookup
     *  @param  type        record type to look up
     *  @param  bits        extra bits to be included in the query
     *  @param  data        optional data (only for type = ns_o_notify)
     *  @throws std::runtime_error
     */
    Lookup(Core *core, Handler *handler, int op, const char *dname, int type, const Bits &bits, const unsigned char *data = nullptr) :
        Operation(handler),
        _core(core),
        _query(op, dname, type, bits, data) {}

public:
    /**
     *  Destructor
     */
    virtual ~Lookup() = default;

    /**
     *  Expose the original query
     *  @return Query
     */
    virtual const Query &query() const override { return _query; }
    
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
     *  How long should we wait until the next runtime?
     *  @param  double      current time
     *  @return double      delay in seconds
     */
    virtual double delay(double now) const = 0;
    
    /**
     *  Execute the next step in the lookup (for example to send out an extra datagram, or to report
     *  a timeout back to userspace). This method returns true when indeed a call to userspace was
     *  done during this call to execute(), and false if further processing is required.
     * 
     *  Note that a lookup can also finish via other execution paths too (for example in a method
     *  that processes responses to UDP lookups). So you can not solely rely on the return value of
     *  this method to find out if a lookup was finished, and you might need to call the finished()
     *  method too to find this out. 
     * 
     *  Watch out: this method should NOT be called when a lookup is already finished!!
     * 
     *  @param  now         current time
     *  @return bool        was a result reported to userspace?
     */
    virtual bool execute(double now) = 0;
};
    
/**
 *  End of namespace
 */
}
