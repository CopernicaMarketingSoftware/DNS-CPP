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
 *  @param  udp     udp
 *  @throws std::runtime_error
 */
Nameserver::Nameserver(Core *core, const Ip &ip, Udp *udp) : _core(core), _ip(ip), _udp(udp) {}

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
    return _udp->send(_ip, query, _core->buffersize());
}

/**
 *  Method that is called when a response is received
 *  @param  buffer      the received response
 *  @param  size        size of the response
 */
void Nameserver::receive(const unsigned char *buffer, size_t size)
{
    // add to the responses
    _responses.emplace_back(buffer, size);
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

            // filter on the response, the beginning is simply the handler at nullptr
            auto begin = _handlers.lower_bound(std::make_pair(response.id(), nullptr));

            // iterate over those elements, notifying each handler
            for (auto iter = begin; iter != _handlers.end(); ++iter) 
            {
                // if this element is not applicable any more, we're going to leap out (we're done)
                if (iter->first != response.id()) break;

                // call the onreceived for the element
                if (iter->second->onReceived(this, response)) result += 1;
                
                // the message was processed, we no longer need other handlers
                break;
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

