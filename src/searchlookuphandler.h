/**
 *  @file searchlookuphandler.h
 *  
 *  This class wraps around a dns callback, and tries search-paths untill either all fail or one succeeds
 *  It then passes the first successfull call, or the last failed call, back to user-space
 *  
 *  @author Bram van den Brink (bram.vandenbrink@copernica.nl)
 *  @date 2022-06-29
 *  @copyright 2022 Copernica BV
 */

#pragma once

#include <dnscpp/context.h>

namespace DNS {

class SearchLookupHandler : public DNS::Handler
{
private:

    /**
     *  Ptr to context to chain calls
     *  @var DNS::Context
     */
    DNS::Context *_context;

    /**
     *  ptr to the original handler, to pass calls along
     *  @var DNS::Handler
     */
    DNS::Handler *_realHandler;

    /**
     *  the base domain, this is the domain becofore any modifications
     *  @var std::string
     */
    const char *_basedomain;

    /**
     *  the search paths, these will be appended to the basedomain and attempted in-order
     *  @var std::vector<std::string>
     */
    std::vector<std::string> _searchPaths;

    /**
     *  the index of searchpaths we are currently trying
     *  @var int
     */
    size_t _index;

    /**
     *  The type of the request
     *  @var ns_type
     */
    ns_type _type;

    /**
     *  The bits of the request
     *  @var DNS::Bits
     */
    const DNS::Bits _bits;

    /**
     *  Method that is called when a valid, successful, response was received.
     * 
     *  This is the method that you normally implement to handle the result
     *  of a resolver-query.
     * 
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onResolved(const Operation *operation, const Response &response) 
    {
        // self destruct, as this query is finished
        delete this;
    }

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
    virtual void onFailure(const Operation *operation, int rcode) 
    {
        // try the next searchpath
        if (tryNextLookup()) return;

        // self destruct, as this query is finished
        delete this;
    }

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
    virtual void onReceived(const Operation *operation, const Response &response)
    {
        // report to user-space
        _realHandler->onReceived(operation, response);

        // pass to base class
        Handler::onReceived(operation, response);
    }

    /**
     *  Method that is called when an operation times out.
     * 
     *  This normally happens when none of the nameservers sent back a response.
     *  The default implementation is for most callers good enough: it passes to call 
     *  on to Handler::onFailure() with a SERVFAIL error.
     * 
     *  @param  operation       the operation that timed out
     */
    virtual void onTimeout(const Operation *operation)
    {
        // return to user-space
        _realHandler->onTimeout(operation);

        // pass to base class
        Handler::onTimeout(operation);
    }
    
    /**
     *  Method that is called when the operation is cancelled
     *  This method is immediately triggered when operation->cancel() is called.
     * 
     *  @param  operation       the operation that was cancelled
     */
    virtual void onCancelled(const Operation *operation) 
    {
        // should it try the next one here?
        _realHandler->onCancelled(operation);

        // self destruct, as this query is cancelled
        delete this;
    }

    public:
    /**
     *  Constructor
     *  @param searchpaths  the searchpaths we need to iterate one by one
     *  @param context      the context object that will perform the querys
     *  @param type         the type of query the user supplied
     *  @param bits         the bits the user supplied
     *  @param basedomain   the base domain the user supplied
     *  @param handler      the handler the user supplied
     */
    SearchLookupHandler(std::vector<std::string> searchpaths, DNS::Context *context, ns_type type, const DNS::Bits bits, const char *basedomain, DNS::Handler* handler) :
        _context(context),
        _realHandler(handler), 
        _basedomain(basedomain),
        _searchPaths(searchpaths),
        _index(0),
        _type(type),
        _bits(bits) {}

    /**
     *  Attempt the next searchpath
     *  @return operation        the operation that handles the lookup
     */
    Operation *tryNextLookup()
    {
        // if there are no more paths left, return null
        if (_index >= _searchPaths.size()) return nullptr;

        // create a string to hold the next domain, and fill it with the user-domain
        std::string nextdomain(_basedomain);

        // add a . and the searchpath
        nextdomain += "." + _searchPaths[_index++];

        // perform dns-query on the constructed path
        return _context->query(nextdomain.c_str(), _type, _bits, this);
    }

    /**
     *  Destructor
     */
    virtual ~SearchLookupHandler() = default;
};


}