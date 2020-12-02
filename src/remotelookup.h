/**
 *  RemoteLookup.h
 * 
 *  Class that encapsulates all data that is needed for a single request
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
#include <memory>
#include "../include/dnscpp/nameserver.h"
#include "../include/dnscpp/timer.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/operation.h"
#include "../include/dnscpp/request.h"
#include "../include/dnscpp/bits.h"
#include "connection.h"
#include "now.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Core;
class Handler;

/**
 *  Class definition
 */
class RemoteLookup : public Operation, private Nameserver::Handler, private Connection::Handler, private Timer
{
private:
    /**
     *  Pointer to the core of the DNS library
     *  @var Core
     */
    Core *_core;

    /**
     *  When was the job started?
     *  @var Now
     */
    Now _started;
    
    /**
     *  Number of messages that have already been sent
     *  @var size_t
     */
    size_t _count = 0;
    
    /**
     *  Identifier of the timer
     *  @var void*
     */
    void *_timer;

    /**
     *  If we got a truncated response, we start a tcp connection to get the full response
     *  @var Connection
     */
    std::unique_ptr<Connection> _connection;
    

    /**
     *  Method that is called when a dgram response is received
     *  @param  nameserver  the reporting nameserver
     *  @param  response    the received response
     */
    virtual bool onReceived(Nameserver *nameserver, const Response &response) override;

    /**
     *  Called when the response has been received over tcp
     *  @param  connection  the reporting connection
     *  @param  response    the received answer
     */
    virtual void onReceived(Connection *connection, const Response &response) override;
    
    /**
     *  Called when the connection could not be used
     *  @param  connector   the reporting connection
     *  @param  response    the original answer (the original truncated one)
     */
    virtual void onFailure(Connection *connection, const Response &truncated) override;

    /**
     *  Method that is called by user space when the timer expires
     */
    virtual void expire() override;

    /**
     *  When does the job expire?
     *  @return double
     */
    double expires() const;

    /** 
     *  Time out the job because no appropriate response was received in time
     */
    void timeout();

    /**
     *  How long should we wait until the next message?
     *  @param  now     current time
     *  @return double
     */
    double delay(double now) const;

    /**
     *  Retry / send a new message to the nameservers
     *  @param  now     current time
     */
    void retry(double now);

    /**
     *  Method to report the response
     *  @param  response
     */
    void report(const Response &response);

    /**
     *  Cleanup the object
     */
    void cleanup();

    /**
     *  Private destructor, the class is self-destructing
     */
    virtual ~RemoteLookup();

public:
    /**
     *  Constructor
     *  @param  core        dns core object
     *  @param  domain      the domain of the lookup
     *  @param  type        type of records to look for
     *  @param  bits        the bits to include in the request
     *  @param  handler     user space object interested in the result
     */
    RemoteLookup(Core *core, const char *domain, ns_type type, const Bits &bits, DNS::Handler *handler);
    
    /**
     *  No copying
     *  @param  that
     */
    RemoteLookup(const RemoteLookup &that) = delete;
};

/**
 *  End of namespace
 */
}
