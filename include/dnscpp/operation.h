/**
 *  Operation.h
 * 
 *  When you make a call to Context::query() a pointer to an operation
 *  object is returned. This pointer can be ignored (it is managed
 *  internally by the DNS library. However, you can store it and 
 *  call methods on it (for example when you are no longer interested
 *  in the operation.
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
#include "query.h"

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
class Operation
{
protected:
    /**
     *  The user-space handler (this is set to nullptr when the result has been reported to userspace)
     *  @var Handler
     */
    Handler *_handler;
            
    /**
     *  Constructor
     *  @param  handler     user space handler
     *  @throws std::runtime_error
     */
    Operation(Handler *handler) : _handler(handler) {}

    /**
     *  Private destructor because userspace is not supposed to destruct this
     */
    virtual ~Operation() = default;

public:
    /**
     *  No copying
     *  @param  that
     */
    Operation(const Operation &that) = delete;

    /**
     *  Expose the original query
     *  @return Query
     */
    virtual const Query &query() const = 0;
    
    /**
     *  Change the handler / install a different object to be notified of changes
     *  @param  handler
     */
    void install(Handler *handler)
    {
        // new handlers cannot be installed when the operation is ready
        if (_handler == nullptr) return;
        
        // update the handler
        _handler = handler;
    }
    
    /**
     *  Cancel the operation
     */
    virtual void cancel() = 0;
};

/**
 *  End of namespace
 */
}

