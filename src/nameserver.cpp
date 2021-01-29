/**
 *  Nameserver.cpp
 * 
 *  Implementation file of the Nameserver class
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/nameserver.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/loop.h"
#include "now.h"


#include <iostream>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  core    the core object with the settings and event loop
 *  @param  ip      nameserver IP
 *  @throws std::runtime_error
 */
Nameserver::Nameserver(Core *core, const Ip &ip) : _loop(core->loop()), _ip(ip), _udp(core, this) {}

/**
 *  Destructor
 */
Nameserver::~Nameserver()
{
    // stop monitoring idle-state
    stop();
}

/**
 *  Helper method to stop monitoring the idle state
 */
void Nameserver::stop()
{
    // nothing to do if not checking for idle
    if (_idle == nullptr) return;

    // cancel the idle watcher
    _loop->cancel(_idle, this);

    // forget the ptr
    _idle = nullptr;
}

/**
 *  Send a datagram to the nameserver
 *  @param  query
 *  @return bool
 */
bool Nameserver::datagram(const Query &query)
{
    // send the message
    return _udp.send(_ip, query);
}

/**
 *  Method that is called when a response is received
 *  @param  address     the address of the nameserver from which it is received
 *  @param  buffer      the received response
 *  @param  size        size of the response
 */
void Nameserver::onReceived(time_t now, const sockaddr *address, const unsigned char *buffer, size_t size)
{
    // parse the address
    Ip ip(address);
    
    // ignore messages not from ip
    if (_ip != ip) return;

    // if nobody is interested there is no point in handling the message
    if (_handlers.empty()) return;

    // add to the responses
    // @todo do not fetch time so often
    _responses.emplace_back(now, address, buffer, size);
    
    // if we're already processing, we don't need to install idle watcher
    if (_idle) return;

    // create a new idle watcher now
    _idle = _loop->idle(this);
}

/**
 *  Process messages when idle
 */
void Nameserver::idle()
{
    // if there is nothing to do, we can stop the watcher (no more responses to feed back)
    if (_responses.empty()) return stop();

    // avoid exceptions (parsing the response could fail)
    try
    {
        // note that the _handler->onReceived() method must be the LAST CALL in this function,
        // because after this call to userspace, "this" could very well have been destructed,
        // so we first have to remove the element from the list, before we call the handler,
        // to do this without copying, we create a list with just one element
        decltype(_responses) oneitem;
        
        // move the first item from the _responses to the one-item list
        oneitem.splice(oneitem.begin(), _responses, _responses.begin(), std::next(_responses.begin()));
        
        // get the first element
        const auto &front = oneitem.front();

        // parse the response
        Response response(front.data(), front.size());
    
        // filter on the response, the beginning is simply the handler at nullptr
        auto begin = _handlers.lower_bound(std::make_pair(response.id(), nullptr));

        // iterate over those elements, notifying each handler
        for (auto iter = begin; iter != _handlers.end(); ++iter) 
        {
            // if this element is not applicable any more, we're going to leap out (we're done)
            if (iter->first != response.id()) return;

            // call the onreceived for the element, if it returns true, it has acknowledged that it
            // processed the response and so we can stop iterating. we have to, since it might have removed
            // itself so iter may be broken at this point.
            if (iter->second->onReceived(this, response)) return;
        }
    }
    catch (const std::runtime_error &error)
    {
        // parsing the response failed
    }
}
    
    
    
/**
 *  End of namespace
 */
}

