/**
 *  Callbacks.h
 * 
 *  Type definition of the callbacks
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
#include <functional>
#include "handler.h"

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Forward declarations
 */
class Response;
class Operation;

/**
 *  The callback-type used for the callback-based query() methods
 *  @type   function
 */
using SuccessCallback = std::function<void(const Operation *, const Response &response)>;
using FailureCallback = std::function<void(const Operation *, int rcode)>;

/**
 *  Utility class to turn callbacks into a handler
 */
class Callbacks : public Handler
{
private:
    /**
     *  The success callback
     *  @var SuccessCallback
     */
    SuccessCallback _success;
    
    /**
     *  The failure callback
     *  @var FailureCallback
     */
    FailureCallback _failure;
    
    
    /**
     *  Method that is called when a valid, successful, response was received.
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onResolved(const Operation *operation, const Response &response) override
    {
        // call to user-space
        _success(operation, response);
        
        // self-destruct
        delete this;
    }

    /**
     *  Method that is called when a query could not be processed or answered. 
     *  @param  operation       the operation that finished
     *  @param  rcode           the received rcode
     */
    virtual void onFailure(const Operation *operation, int rcode) override
    {
        // call to user-space
        _failure(operation, rcode);

        // self-destruct
        delete this;
    }

    /**
     *  Method that is called when the operation is cancelled
     *  @param  operation       the operation that was cancelled
     */
    virtual void onCancelled(const Operation *operation) override
    {
        // self-destruct
        delete this;
    }
    
    /**
     *  Private destructor (object is self-destructing)
     */
    virtual ~Callbacks() = default;

public:
    /**
     *  Constructor
     *  @param  success     the success-callback
     *  @param  failure     the failure-callback
     */
    Callbacks(const SuccessCallback &success, const FailureCallback &failure) :
        _success(success), _failure(failure) {}
};
    
/**
 *  End of namespace
 */
}
