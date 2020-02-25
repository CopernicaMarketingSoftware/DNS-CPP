/**
 *  A.cpp
 * 
 *  Simple test program to retrieve an A record
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include <ev.h>
#include <dnscpp.h>
#include <dnscpp/libev.h>
#include <iostream>


#include "../src/name.h"
#include "../src/input.h"


/**
 *  The validator is a handler that checks whether the public key 
 *  matches the RRSIG
 */
class Validator : public DNS::Handler
{
private:
    /**
     *  We need to keep the response in scope
     *  @var DNS::Response
     */
    DNS::Response _response;

    /**
     *  The record that holds the RRSIG
     *  @var DNS::Answer
     */
    DNS::Answer _record;

    /**
     *  The signature that is being validated
     *  @var DNS::RRSIG
     */
    DNS::RRSIG _rrsig;



    /**
     *  Helper function locates the record number that holds the signature
     *  @param  response
     *  @return index
     *  @throws std::runtime_error
     */
    static int rrsigno(const DNS::Response &response)
    {
        // check all records
        for (size_t i = 0; i < response.answers(); ++i)
        {
            // read out the record
            DNS::Answer record(response, i);
            
            // is this the appropriate type?
            if (record.type() == ns_t_rrsig) return i;
        }
        
        // not found
        throw std::runtime_error("no RRSIG found");
    }

    /**
     *  Private destructor because object is self-destructing
     */
    virtual ~Validator() = default;

public:
    /**
     *  Constructor
     *  @param  context         the context
     *  @param  response        the original response
     */
    Validator(DNS::Context *context, const DNS::Response &response) : 
        _response(response), _record(response, rrsigno(response)), _rrsig(response, _record) 
    {
        // start a new query
        context->query(_rrsig.signer(), ns_t_dnskey, this);
        
        // tell what we're doing
        std::cout << "look for dnskey with id " << _rrsig.keytag() << " in zone " << _rrsig.signer() << std::endl;
    }

    /**
     *  Method that is called when a raw response is received
     *  @param  response        the received response
     */
    virtual void onReceived(const DNS::Response &response) override
    {
        std::cout << "validation response is in" << std::endl;
        
        // go look for the DNSKEY record
        for (size_t i = 0; i < response.answers(); ++i)
        {
            // read out the record
            DNS::Answer record(response, i);
            
            // is this indeed an DNSKEY record?
            if (record.type() != ns_t_dnskey) break;
            
            // parse as DNSKEY record
            DNS::DNSKEY dnskey(response, record);
            
            // according to the specs, we should ignore keys that are not zone-keys
            if (!dnskey.zonekey()) continue;
            
            // and the protocol must be 3
            if (dnskey.protocol() != 3) continue;
            
            // @todo also compare owner-name?
            
            // are we using the same algorithm
            if (dnskey.algorithm() != _rrsig.algorithm()) continue;
            
            // are the keytags the same?
            if (dnskey.keytag() != _rrsig.keytag()) continue;
            
            
            
            // report about the DNSKEY record
            std::cout << "- dnskey record " << dnskey.name() << " " << dnskey.ttl() << " " << dnskey.keytag() << std::endl;
        }
        
    }
};

/**
 *  Handler that waits for the response on 
 */
class Receiver : public DNS::Handler
{
private:
    /**
     *  Pointer to the original context
     *  @var DNS::Context
     */
    DNS::Context *_context;

public:
    /**
     *  Constructor
     *  @param  context
     */
    Receiver(DNS::Context *context) : _context(context) {}

    /**
     *  Method that is called when a raw response is received
     *  @param  response        the received response
     */
    virtual void onReceived(const DNS::Response &response) override
    {
        // go look for the A record
        for (size_t i = 0; i < response.answers(); ++i)
        {
            // read out the record
            DNS::Answer record(response, i);
            
            // is this indeed an A record?
            if (record.type() != ns_t_a) break;
            
            // parse as A record
            DNS::A a(response, record);
            
            // report about the A record
            std::cout << "- a record " << a.name() << " " << a.ttl() << " " << a.ip() << std::endl;
        }
        
        // and now we're going to validate this record
        new Validator(_context, response);
    }
};

/**
 *  Main procedure
 *  @return int
 */
int main()
{
    // the event loop
    struct ev_loop *loop = EV_DEFAULT;
    
    // wrap the loop to make it accessible by dns-cpp
    DNS::LibEv myloop(loop);
    
    // create a dns context
    DNS::Context context(&myloop);

    // we need a handler
    Receiver handler(&context);

    // add a nameserver
    context.nameserver(DNS::Ip("8.8.8.8"));
    
    // do a query
    context.query("www.example.com", ns_t_a, &handler);
    
    // run the event loop
    ev_run(loop, 0);
    
    // done
    return 0;
}
