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
class ARecords;
class AAAARecords;
class MXRecords;
class CNAMERecords;
class Query;
class Operation;

/**
 *  Class definition
 */
class Handler
{
public:
    /**
     *  Method that is called when an operation times out.
     * 
     *  This normally happens when none of the nameservers sent back a response.
     *  The default implementation passes to call on to Handler::onError().
     *  Override this method if you want to distinguish between timeouts
     *  and other errors.
     * 
     *  @param  operation       the operation that timed out
     *  @param  query           the query that was attempted
     */
    virtual void onTimeout(const Operation *operation, const Query &query);
    
    /**
     *  Method that is called when a raw response is received. This includes
     *  successful responses, error responses and truncated responses.
     * 
     *  The default implementation inspects the response. If it is OK it passes
     *  it on to the onSuccess() method, and when it contains an error of any
     *  sort it calls the onError() method.
     * 
     *  @param  operation       the operation that finished
     *  @param  query           the original query
     *  @param  response        the received response
     */
    virtual void onResponse(const Operation *operation, const Query &query, const Response &response);

    /**
     *  Method that is called when a query could not be processed or answered. 
     * 
     *  If the RCODE is servfail it could mean multiple things: the nameserver sent
     *  back an actual SERVFAIL response, or we were unable to reach the nameserver,
     *  or unable to parse the response. 
     * 
     *  @param  operation       the operation that finished
     *  @param  query           the original query
     *  @param  rcode           the received rcode
     */
    virtual void onError(const Operation *operation, const Query &query, int rrcode) {}

    /**
     *  Method that is called when a valid response was received.
     * 
     *  The default implementation inspects the response and passes it on to one of the
     *  many (and most appropriate) onReceived() methods.
     * 
     *  @param  operation       the operation that finished
     *  @param  query           the original query
     *  @param  response        the received response
     */
    virtual void onSuccess(const Operation *operation, const Query &query, const Response &response);

    /**
     *  When you made a call for specific records, you can implement
     *  one or more of the following methods to get exactly the information
     *  that you were looking for.
     *
     *  @param  operation       the operation that finished
     *  @param  hostname        the hostname for which we were looking for information
     *  @param  records         the received records
     */
    virtual void onReceived(const Operation *operation, const char *hostname, const ARecords &records) {}
    virtual void onReceived(const Operation *operation, const char *hostname, const AAAARecords &records) {}
    virtual void onReceived(const Operation *operation, const char *hostname, const MXRecords &records) {}
    virtual void onReceived(const Operation *operation, const char *hostname, const CNAMERecords &records) {}
};

/**
 *  End of namespace
 */
}

