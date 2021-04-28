/**
 *  Socket.cpp
 * 
 *  Implementation file for the Socket class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/socket.h"
#include "../include/dnscpp/watcher.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/processor.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Add a message for delayed processing
 *  @param  addr    the address from which the message was received
 *  @param  buffer  the response buffer
 */
void Socket::add(const sockaddr *addr, std::vector<unsigned char> &&buffer)
{
    // avoid exceptions (in case the ip cannot be parsed)
    try
    {
        // @todo inbound messages that do not come from port 53 can be ignored

        // remember the response for now
        _responses.emplace_back(addr, move(buffer));
    }
    catch (...)
    {
        // ip address could not be parsed
    }

    // reschedule the processing of messages
    _handler->onActive(this);
}

/**
 *  Add a message for delayed processing
 *  @param  addr    the address from which the message was received
 *  @param  buffer  the response buffer
 */
void Socket::add(const Ip &addr, std::vector<unsigned char> &&buffer)
{
    // add to the list
    _responses.emplace_back(addr, move(buffer));

    // reschedule the processing of messages
    _handler->onActive(this);
}

/**
 *  Invoke callback handlers for buffered raw responses
 *  @param      watcher   The watcher to keep track if the parent object remains valid
 *  @param      maxcalls  The max number of callback handlers to invoke
 *  @return     number of callback handlers invoked
 */
size_t Socket::process(size_t maxcalls)
{
    // the number of callback handlers invoked
    size_t result = 0;
    
    // use a watcher in case object is destructed in the meantime
    Watcher watcher(this);

    // look for a response
    while (result < maxcalls && watcher.valid() && !_responses.empty())
    {
        // avoid exceptions (parsing the response could fail)
        try
        {
            // note that the _handler->onReceived() triggers a call to user-space that might destruct 'this',
            // which also causes responses to be destructed. To avoid silly crashes we copy the oldest message
            // to the local stack in a one-item-big list
            decltype(_responses) oneitem;

            // move the first item from the responses to the one-item list
            oneitem.splice(oneitem.begin(), _responses, _responses.begin(), std::next(_responses.begin()));

            // get the first element
            const auto &front = oneitem.front();

            // parse the response
            Response response(front.second.data(), front.second.size());

            // make it known that this ID is now free to use
            onReceivedId(response.id());

            // filter on the response, the beginning is simply the handler at nullptr
            auto begin = _processors.lower_bound(std::make_tuple(response.id(), front.first, nullptr));

            // iterate over those elements, notifying each handler
            for (auto iter = begin; iter != _processors.end(); ++iter)
            {
                // if this element is not applicable any more, we're going to leap out (we're done)
                if (std::get<0>(*iter) != response.id() || std::get<1>(*iter) != front.first) break;

                // call the onreceived for the element
                if (std::get<2>(*iter)->onReceived(front.first, response)) result += 1;

                // the message was processed, we no longer need other handlers
                break;
            }
        }
        catch (const std::runtime_error &error)
        {
            // parsing the response failed, or the callback handler threw an exception
        }
    }

    // done
    return result;
}

/**
 *  End of namespace
 */
}

