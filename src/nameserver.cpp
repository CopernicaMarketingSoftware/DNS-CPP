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
#include "../include/dnscpp/now.h"
#include "../include/dnscpp/watcher.h"

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
Nameserver::Nameserver(Core *core, const Ip &ip) : _core(core), _ip(ip), _udp(core, this) {}

/**
 *  Destructor
 */
Nameserver::~Nameserver() {}

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
    _responses.emplace_back(address, buffer, size);
    
    // let the core that we need to process this queue
    _core->reschedule(now);
}

/**
 *  Process queued messages
 *  @param  size_t          max number of calls to userspace
 *  @return size_t          number of processed answers
 */
size_t Nameserver::process(size_t maxcalls)
{
    // if there is nothing to process
    if (_responses.empty()) return 0;
    
    // result variable
    size_t result = 0;
    
    // we are going to make a call to userspace, so we keep monitoring if `this` is not destructed
    Watcher watcher(this);
    
    // look for a response
    while (result < maxcalls && watcher.valid() && !_responses.empty())
    {
        // avoid exceptions (parsing the response could fail)
        try
        {
            // note that the _handler->onReceived() triggers a call to user-space that might destruct 'this',
            // which also causes _responses to be destructed. To avoid silly crashes we copy the oldest message
            // to the local stack in a one-item-big list
            decltype(_responses) oneitem;
            
            // move the first item from the _responses to the one-item list
            oneitem.splice(oneitem.begin(), _responses, _responses.begin(), std::next(_responses.begin()));
            
            // get the first element
            const auto &front = oneitem.front();

            // parse the response
            Response response(front.data(), front.size());

            // find the handler for this query id
            auto iter = _handlers.find(response.id());

            // if there's a handler
            if (iter != _handlers.end())
            {
                // if the handler is not a nullptr
                if (iter->second)
                {
                    // invoke the callback of the handler
                    // note that this handler may mutate our _handlers member variable
                    iter->second->onReceived(this, response);

                    // we have handled a response, so we increment a number
                    result += 1;
                }
            }
        }
        catch (const std::runtime_error &error)
        {
            // parsing the response failed, or the callback handler threw an exception
        }
    }

    // something was processed
    return result;
}
    
/**
 *  End of namespace
 */
}

