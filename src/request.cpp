/**
 *  Request.cpp
 *  
 *  Implementation file for the Requet class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "request.h"
#include "connection.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/answer.h"
#include "../include/dnscpp/handler.h"


// @todo remove this
#include "../include/dnscpp/a.h"
#include "../include/dnscpp/cname.h"


#include <iostream>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  core        dns core object
 *  @param  domain      the domain of the lookup
 *  @param  type        the type of the request
 *  @param  handler     user space object
 */
Request::Request(Core *core, const char *domain, ns_type type, DNS::Handler *handler) : 
    _core(core), 
    _query(ns_o_query, domain, type),
    _handler(handler)
{
    // iterate over the nameservers because we will send a datagram to each one of them
    for (auto &nameserver : core->nameservers())
    {
        // send a datagram, and register ourselves as subscriber
        nameserver.datagram(_query);
        
        // we want to be notified when a response comes in
        nameserver.subscribe(this);
    }
    
    // we set a timer to repeat the call in one second
    // @todo dont use a hardcoded value
    _timer = core->loop()->timer(1.0, this);
    
    // @todo set up a timer to repeat the call
}

/**
 *  Destructor
 */
Request::~Request()
{
    // unsubscribe from the nameservers
    for (auto &nameserver : _core->nameservers()) nameserver.unsubscribe(this);
    
    // stop the timer
    if (_timer) _core->loop()->cancel(_timer, this);
}

/**
 *  When does the request expire?
 *  @return double
 */
double Request::expires() const
{
    // get the max time
    return _started + _core->expire();
}

/** 
 *  Time out the request because no appropriate response was received in time
 */
void Request::timeout()
{
    // report an error
    _handler->onFailure(this);
    
    // self-destruct
    delete this;
}

/**
 *  Retry / send a new message to the nameservers
 *  @param  now     current timestamp
 */
void Request::retry(double now)
{
    // send a datagram to each nameserver
    for (auto &nameserver : _core->nameservers())
    {
        // send a datagram, and register ourselves as subscriber
        nameserver.datagram(_query);
    }
    
    // when is the next attempt?
    double next = std::min(now + _core->interval(), expires());

    // we set a new timer for when the entire request times out
    _timer = _core->loop()->timer(next - now, this);
}

/**
 *  When the timer expires
 *  This method is called from the event loop in user space
 */
void Request::expire()
{
    // find the current time
    Now now;
    
    // the timer has expired
    _timer = nullptr;
    
    // did the entire request expire?
    if (now >= expires()) return timeout();
    
    // if we do not yet have a tcp connection we send out more dgrams
    if (!_connection) return retry(now);
    
    // we set a new timer for when the entire request times out
    _timer = _core->loop()->timer(expires() - now, this);
}

/**
 *  Method that is called when a response is received
 *  @param  nameserver  the reporting nameserver
 *  @param  response    the received response
 */
void Request::onReceived(Nameserver *nameserver, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return;
    
    // if we're already busy with a tcp connection we ignore further dgram responses
    if (_connection) return;
    
    // if the response was truncated, we ignore it and start a tcp connection
    if (response.truncated()) return _connection.reset(new Connection(_core->loop(), nameserver->ip(), _query, this));
    
    // we have a response, so we can pass that to user space
    _handler->onReceived(response);
    
    // we can self-destruct -- this request has been handled
    delete this;
}

/**
 *  Called when the response has been received
 *  @param  connection
 *  @param  response
 */
void Request::onReceived(Connection *connection, const Response &response)
{
    // ignore responses that do not match with the query
    // @todo should we check for more? like whether the response is indeed a response
    if (!_query.matches(response)) return;
    
    // we have a response, hand it over to user space
    _handler->onReceived(response);
    
    // self-destruct now that the request has been completed
    delete this;



    std::cout << "response over tcp" << std::endl;

    std::cout << "received response " << response.id() << std::endl;
    
    std::cout << "questions: " << response.questions() << std::endl;
    std::cout << "answers: " << response.answers() << std::endl;
    std::cout << "nameservers: " << response.nameservers() << std::endl;
    std::cout << "additional: " << response.additional() << std::endl;

    for (size_t i = 0; i < response.answers(); ++i)
    {
        Answer record(response, i);
        
        std::cout << "answer: " << record.type() << " " << record.name() << std::endl;
        
        if (record.type() == ns_t_a) std::cout << "    ip: " << A(response, record).ip() << std::endl;
        if (record.type() == ns_t_cname) std::cout << "    target: " << CNAME(response, record).target() << std::endl;
    }
    
    // @todo different implementation
}

/**
 *  Called when the connection could not be used
 *  @param  connection
 */
void Request::onFailure(Connection *connection)
{
    std::cout << "tcp failure" << std::endl;
    
    
}


    
/**
 *  End of namespace
 */
}

