/**
 *  SearchLookup.h
 *  
 *  This class wraps around a dns callback, and tries search-paths untill either all fail or one succeeds
 *  It then passes the first successfull call, or the last failed call, back to user-space
 *  
 *  @author Bram van den Brink (bram.vandenbrink@copernica.nl)
 *  @copyright 2022 - 2025 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <dnscpp/context.h>
#include "../include/dnscpp/response.h"
#include <optional>

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
     *  The authority to use
     *  @var std::shared_ptr<Authority>
     */
    std::shared_ptr<Authority> _authority;

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
     *  Optional empty response (with no matching records) that we found for one of the
     *  lookups in the process (because we rather report an empty record set than a NXDOMAIN
     *  result, because we want to tell the caller that at least *something* exists).
     *  @var Response
     */
    std::optional<Response> _response;


    /**
     *  Cache the response for later use
     *  @param  response
     */
    void cache(const Response &response)
    {
        // no need to cache if we already have something, and invalid responses are not cached either
        if (_response || response.rcode() != 0) return;
        
        // cache the response
        _response.emplace(response);
    }

    /**
     *  Method that is called when a raw response is received
     *  @param  operation       the reporting operation
     *  @param  response        the received response
     */
    virtual void onReceived(const Operation *operation, const Response &response) override
    {
        // an mxdomain error should trigger a next lookup
        if (response.rcode() == ns_r_nxdomain && proceed()) return;
        
        // if there are no matching records, we also want to do the next lookup,
        // but we do remember this empty response because it could be better than the final nxdomain
        if (response.records(ns_s_an, _type) == 0 && proceed()) return cache(response);

        // pass on to user-space (note that we do not pass the nxdomain response if we have a better alternative)
        _handler->onReceived(this, response.rcode() != ns_r_nxdomain || !_response ? response : *_response);
        
        // self destruct, as this query is finished
        delete this;
    }

    /**
     *  Method that is called when the operation is cancelled
     *  This method is immediately triggered when operation->cancel() is called.
     * 
     *  @param  operation       the operation that was cancelled
     */
    virtual void onCancelled(const Operation *operation) override
    {
        // should it try the next one here?
        _handler->onCancelled(operation);

        // self destruct, as this query is cancelled
        delete this;
    }

    /**
     *  Method that is called when an operation times out.
     *  @param  operation       the operation that timed out
     *  @param  query           the query that was attempted
     */
    virtual void onTimeout(const Operation *operation) override
    {
        // pass on to user-space
        _handler->onTimeout(this);
        
        // operation is ready
        delete this;
    }

    /**
     *  Attempt the next searchpath
     *  @return operation        the operation that handles the lookup
     */
    bool proceed()
    {
        // do nothing if we already have exhausted all calls
        if (_index == size_t(-1)) return false;

        // shortcut to the search-paths
        const auto &searchpaths = _authority->searchpaths();
        
        // if there are no more paths left, return false
        if (_index >= searchpaths.size()) return finalize();

        // the next path to check
        const auto &nextdomain = searchpaths[_index++];
        
        // for empty domains we do not need to concatenate
        if (nextdomain.empty()) return finalize();

        // create a string to hold the next domain, and fill it with the user-domain
        std::string nexthost(_basedomain);

        // add a . and the searchpath
        nexthost.append(".").append(nextdomain);

        // perform dns-query on the constructed path
        // and save the operation so we can cancell it if requested
        _operation = _context->query(nexthost.c_str(), _type, _bits, this);

        // return success
        return true;
    }

    /**
     *  Do the ultimate, final lookup
     *  @return bool
     */
    bool finalize()
    {
        // start a lookup for just the requested domain
        _operation = _context->query(_basedomain.c_str(), _type, _bits, this);
        
        // update index to prevent loops
        _index = size_t(-1);
        
        // still busy
        return true;
    }

    /**
     *  Private destructor because this is a self-destructing object
     */
    virtual ~SearchLookup() = default;

public:
    /**
     *  Constructor
     *  @param context      the context object that will perform the queries
     *  @param authority    object that knows which search-path to use
     *  @param type         the type of query the user supplied
     *  @param bits         the bits the user supplied
     *  @param basedomain   the base domain the user supplied
     *  @param handler      the handler the user supplied
     */
    SearchLookup(DNS::Context *context, const std::shared_ptr<Authority> &authority, ns_type type, const DNS::Bits bits, const char *basedomain, DNS::Handler* handler) :
        Operation(handler),
        _context(context),
        _authority(authority),
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
