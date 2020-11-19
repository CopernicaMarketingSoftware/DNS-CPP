/**
 *  Nameserver.cpp
 * 
 *  Implementation file for the Nameserver class
 * 
 *  @author Michael van der Werve <michael.vanderwerve@mailerq.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/nameserver.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/query.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Method that is called when a response is received
 *  @param  ip          the ip of the nameserver from which it is received
 *  @param  buffer      the received response
 *  @param  size        size of the response
 */
void Nameserver::onReceived(const Ip &ip, const unsigned char *buffer, size_t size)
{
    // avoid exceptions (parsing the response could fail)
    try
    {
        // ignore responses from other ips
        // @todo also ignore messages that do not come from port 53???
        if (ip != _ip) return;
        
        // if nobody is interested there is no point in parsing the message
        if (_handlers.empty()) return;
        
        // parse the response
        Response response(buffer, size);
    
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
    catch (...)
    {
        // parsing the response failed
    }
}

/**
 *  Send a datagram to the nameserver
 *  @param  query
 *  @return bool
 */
bool Nameserver::datagram(const Query &query)
{
    // send the package
    return _udp[query.id() % _udp.size()]->send(_ip, query);
}

/**
 *  Subscribe to the socket if you want to be notified about incoming responses
 *  @param  handler     the handler that wants to receive an answer
 *  @param  id          id of the response that the handler is interested in
 */
void Nameserver::subscribe(Handler *handler, uint16_t id)
{
    // emplace the handler
    _handlers.insert(std::make_pair(id, handler));

    // nothing to do if already enough sockets open, leap out
    if (_udp.size() >= _core->sockets()) return;

    // nothing to do if not at the average cap yet, leap out
    if (_handlers.size() > _core->socketrequests() * _udp.size()) return;

    // create a new socket
    _udp.emplace_back(new Udp(_core, this));
}

/**
 *  Unsubscribe from the socket, this is the counterpart of subscribe()
 *  @param  handler     the handler that unsubscribes
 *  @param  id          id of the response that the handler is interested in
 */
void Nameserver::unsubscribe(Handler *handler, uint16_t id)
{
    // find the element
    auto iter = _handlers.find(std::make_pair(id, handler));

    // if it is not found, we leap out (this should not happen)
    if (iter == _handlers.end()) return;

    // simply erase the element
    _handlers.erase(iter);

    // @todo in the future, we might want to close udp sockets again when we have
    //       too much capacity, but we can address this when this becomes an issue.
    //       for now we just make sure it gets properly cleaned up.

    // if there are still handlers listening, leap out
    if (!_handlers.empty()) return;

    // clear everything past the begin
    _udp.erase(++_udp.begin(), _udp.end());

    // close the first socket as well, will be opened again later
    _udp[0]->close();
}

/**
 *  End of namespace
 */
}
