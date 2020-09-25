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

/**
 *  Class definition
 */
class Handler
{
public:
    /**
     *  Method that is called when an operation times out.
     * 
     *  This normally happens when none of the nameservers send back a response.
     * 
     *  @param  query           the query that was attempted
     */
    virtual void onTimeout(const Query &query) {}
    
    /**
     *  Method that is called when a raw response is received
     * 
     *  The default implementation of this method reads out the properties
     *  of the response, and passes it on to one of the other, more specific,
     *  onReceived() methods. However, if you want to read out the original response 
     *  yourself, you can override this method.
     * 
     *  Watch out: the response might be truncated in case the nameserver was
     *  not capable of handling a TCP connections (normally all truncated responses
     *  are retried over TCP, but when this fails you still receive the truncated
     *  response).
     * 
     *  @param  query           the original query
     *  @param  response        the received response
     */
    virtual void onReceived(const Query &query, const Response &response);
    
    /**
     *  When you made a call for specific records, you can implement
     *  one or more of the following methods to get exactly the information
     *  that you were looking for.
     *
     *  @param  hostname        the hostname for which we were looking for information
     *  @param  records         the received records
     */
    virtual void onReceived(const char *hostname, const ARecords &records) {}
    virtual void onReceived(const char *hostname, const AAAARecords &records) {}
    virtual void onReceived(const char *hostname, const MXRecords &records) {}
    virtual void onReceived(const char *hostname, const CNAMERecords &records) {}
};

/**
 *  End of namespace
 */
}

