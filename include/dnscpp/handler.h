/**
 *  Handler.h
 * 
 *  In user space programs you should implement the handler interface
 *  that receives the responses to your queries.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */
 
/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Response;
class Query;
class Operation;

/**
 *  Class definition
 */
class Handler
{
public:
    /**
     *  Method that is called when a valid, successful, response was received.
     * 
     *  This is the method that you normally implement to handle the result
     *  of a resolver-query.
     * 
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onResolved(const Operation *operation, const Response &response) {}

    /**
     *  Method that is called when a query could not be processed or answered. 
     * 
     *  If the RCODE is servfail it could mean multiple things: the nameserver sent
     *  back an actual SERVFAIL response, or is was impossible to reach the nameserver,
     *  or the response from the nameserver was invalid.
     * 
     *  If you want more detailed information about the error, you can implement the
     *  low-level onReceived() or onTimeout() methods).
     * 
     *  @param  operation       the operation that finished
     *  @param  rcode           the received rcode
     */
    virtual void onFailure(const Operation *operation, int rcode) {}

    /**
     *  Method that is called when an operation times out.
     * 
     *  This normally happens when none of the nameservers sent back a response.
     *  The default implementation is for most callers good enough: it passes to call 
     *  on to Handler::onFailure() with a SERVFAIL error.
     * 
     *  @param  operation       the operation that timed out
     */
    virtual void onTimeout(const Operation *operation);
    
    /**
     *  Method that is called when a raw response is received. This includes
     *  successful responses, error responses and truncated responses.
     * 
     *  The default implementation inspects the response. If it is OK it passes
     *  it on to the onResolved() method, and when it contains an error of any
     *  sort it calls the onFailure() method.
     * 
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onReceived(const Operation *operation, const Response &response);
    
    /**
     *  Method that is called when the operation is cancelled
     *  This method is immediately triggered when operation->cancel() is called.
     * 
     *  @param  operation       the operation that was cancelled
     */
    virtual void onCancelled(const Operation *operation) {}
};

/**
 *  End of namespace
 */
}

