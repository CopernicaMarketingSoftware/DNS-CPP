/**
 *  SearchLookup.h
 *  
 *  This class wraps around a dns callback, and tries search-paths untill either all fail or one succeeds
 *  It then passes the first successfull call, or the last failed call, back to user-space
 *  
 *  @author Bram van den Brink (bram.vandenbrink@copernica.nl)
 *  @date 2022-06-29
 *  @copyright 2022 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <dnscpp/context.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class SearchLookup : public DNS::Operation, public DNS::Handler
{
private:
    /**
     *  DNS context object to ask for querys
     *  @var DNS::Context
     */
    DNS::Context *_context;

    /**
     *  the base domain, this is the domain becofore any modifications
     *  @var std::string
     */
    std::string _basedomain;

    /**
     *  the index of searchpaths we are currently trying
     *  @var size_t
     */
    size_t _index = 0;

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
     *  The operation we are currently attempting
     *  @var Operation
     */
    Operation *_operation = nullptr;

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
        // report to user-space
        _handler->onResolved(this, response);
        
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
        if (proceed()) return;
        
        // @todo there are multiple sort of failures, should each one trigger a retry????
        
        // no more operations are possible, report to user-space
        _handler->onFailure(this, rcode);
        
        // self destruct, as this query is finished
        delete this;
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
        _handler->onCancelled(operation);

        // self destruct, as this query is cancelled
        delete this;
    }

    /**
     *  Attempt the next searchpath
     *  @return operation        the operation that handles the lookup
     */
    bool proceed()
    {
        // shortcut to the search-paths
        const auto &searchpaths = _context->searchpaths();
        
        // if there are no more paths left, return false
        if (_index >= searchpaths.size()) return false;

        // the next path to check
        const auto &nextdomain = searchpaths[_index++];
        
        // for empty domains we do not need to concatenate
        if (nextdomain.empty())
        {
            // start a lookup for just the requested domain
            _operation = _context->query(_basedomain.c_str(), _type, _bits, this);
        }
        else
        {
            // create a string to hold the next domain, and fill it with the user-domain
            std::string nextdomain(_basedomain);

            // add a . and the searchpath
            nextdomain.append(".").append(searchpaths[_index++]);

            // perform dns-query on the constructed path
            // and save the operation so we can cancell it if requested
            _operation = _context->query(nextdomain.c_str(), _type, _bits, this);
        }

        // return success
        return true;
    }

    /**
     *  Private destructor because this is a self-destructing object
     */
    virtual ~SearchLookup() = default;

public:
    /**
     *  Constructor
     *  @param context      the context object that will perform the querys
     *  @param type         the type of query the user supplied
     *  @param bits         the bits the user supplied
     *  @param basedomain   the base domain the user supplied
     *  @param handler      the handler the user supplied
     */
    SearchLookup(DNS::Context *context, ns_type type, const DNS::Bits bits, const char *basedomain, DNS::Handler* handler) :
        Operation(handler),
        _context(context),
        _basedomain(basedomain),
        _index(0),
        _type(type),
        _bits(bits) 
    {
        // we start the lookup
        proceed();
    }

    /**
     *  Expose the original query
     *  @return Query
     */
    virtual const Query &query() const override { return _operation->query(); }

    /**
     *  Cancel the operation
     */
    virtual void cancel() override
    {
        // cancell the currently active operation
        _operation->cancel();
    }
};

/**
 *  End of namespace
 */
}
