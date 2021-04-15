/**
 *  RemoteLookup.h
 * 
 *  Class that encapsulates all data that is needed for a single request
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
#include <memory>
#include "../include/dnscpp/nameserver.h"
#include "../include/dnscpp/timer.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/lookup.h"
#include "../include/dnscpp/request.h"
#include "../include/dnscpp/bits.h"
#include "../include/dnscpp/now.h"
#include "../include/dnscpp/processor.h"
#include "connection.h"

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
class RemoteLookup : public Lookup, private Connection::Handler, private Processor
{
private:
    /**
     *  Pointer to the core of the DNS library
     *  @var Core
     */
    Core *_core;

    /**
     *  When was the last time that the job ran?
     *  @var double
     */
    double _last = 0.0;
    
    /**
     *  Number of messages that have already been sent
     *  @var size_t
     */
    size_t _count = 0;
    
    /**
     *  Random ID (mainly used to decide which nameserver to use first)
     *  @var size_t
     */
    size_t _id;
    
    /**
     *  If we got a truncated response, we start a tcp connection to get the full response
     *  @var Connection
     */
    std::unique_ptr<Connection> _connection;
    
    /**
     *  Objects to which we're subscribed for inbound messages
     *  @var std::set
     */
    std::set<std::pair<Inbound*,Ip>> _subscriptions;


    /**
     *  Method that is called when a dgram response is received
     *  @param  ip          the ip from where the response came (nameserver ip)
     *  @param  response    the received response
     *  @return bool        was the response processed?
     */
    virtual bool onReceived(const Ip &ip, const Response &response) override;

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
     *  Execute the lookup
     *  @param  now         current time
     *  @return bool        should the lookup be rescheduled?
     */
    virtual bool execute(double now) override;

    /**
     *  When does the job expire?
     *  @return double
     */
    double expires() const;

    /** 
     *  Time out the job because no appropriate response was received in time
     *  @return bool
     */
    bool timeout();

    /**
     *  Wait for internal buffers to catch up (dns-cpp uses an internal buffer
     *  that may hold the response, but that is not yet parsed)
     *  @param  expire      expire time
     *  @return bool
     */
    bool wait(double expires);

    /**
     *  How long should we wait until the next message?
     *  @param  now     current time
     *  @return double
     */
    virtual double delay(double now) const override;

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
     *  @return Handler
     */
    DNS::Handler *cleanup();

    /**
     *  How many credits are left (meaning: how many datagrams do we still have to send?)
     *  @return size_t      number of attempts
     */
    virtual size_t credits() const override;

    /**
     *  Cancel the operation
     */
    virtual void cancel() override;

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

    /**
     *  Destructor
     */
    virtual ~RemoteLookup();

};

/**
 *  End of namespace
 */
}
