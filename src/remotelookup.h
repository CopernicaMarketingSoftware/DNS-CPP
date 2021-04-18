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
#include <set>
#include "../include/dnscpp/timer.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/lookup.h"
#include "../include/dnscpp/request.h"
#include "../include/dnscpp/bits.h"
#include "../include/dnscpp/now.h"
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/processor.h"
#include "connector.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Forward declarations
 */
class Core;
class Handler;
class Inbound;

/**
 *  Class definition
 */
class RemoteLookup : public Lookup, private Processor, private Connector
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
     *  If we move to TCP modus because of truncation, we remember the truncated
     *  response in case the TCP attempt fails so that we can at least report something
     *  @var std::unique_ptr<Response>
     */
    std::unique_ptr<Response> _truncated;
    
    /**
     *  Objects to which we're subscribed for inbound messages
     *  @var std::set
     */
    std::set<std::pair<Inbound*,Ip>> _subscriptions;


    /**
     *  Method that is called when a dgram response is received
     *  @param  ip          the ip from where the response came (nameserver ip)
     *  @param  response    the received response
     *  @return bool        was the response processed, meaning: was it sent back to userspace?
     */
    virtual bool onReceived(const Ip &ip, const Response &response) override;

    /**
     *  Called when a TCP connection has been set up (in case an earlier UDP response was truncated)
     *  @param  ip          ip to which a connection was set up
     *  @param  tcp         the actual TCP connection
     *  @return bool        was there a call to userspace?
     */
    virtual bool onConnected(const Ip &ip, Tcp *tcp) override;
    
    /**
     *  Called when a TCP connection could not be set up
     *  @param  ip          ip to which a connection was set up
     *  @return bool        was there a call to userspace?
     */
    virtual bool onFailure(const Ip &ip) override;

    /**
     *  Execute the lookup
     *  @param  now         current time
     *  @return bool        was there a call back to userspace?
     */
    virtual bool execute(double now) override;

    /**
     *  When does the job expire?
     *  @return double
     */
    double expires() const;

    /** 
     *  Time out the job because no appropriate response was received in time
     *  @return bool        was there a call to user space?
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
     *  @return bool
     */
    bool report(const Response &response);

    /**
     *  Cleanup the object
     *  @return Handler
     */
    DNS::Handler *cleanup();

    /**
     *  Is this lookup still scheduled: meaning that no requests has been sent yet
     *  @return bool
     */
    virtual bool scheduled() const override;
    
    /**
     *  Is this lookup already finished: meaning that a result has been reported back to userspace
     *  @return bool
     */
    virtual bool finished() const override;
    
    /**
     *  Is this lookup exhausted: meaning that it has sent its max number of requests, but still
     *  has not received an appropriate answer, and is now waiting for its final timer to finish
     *  @return bool
     */
    virtual bool exhausted() const override;

    /**
     *  Cancel the operation
     */
    virtual void cancel() override;
    
    /**
     *  Method to unsubscribe from all UDP inbound sockets
     */
    void unsubscribe();

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
