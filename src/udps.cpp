/**
 *  Udp.cpp
 * 
 *  Implementation file for the Udp class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/udps.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/query.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/now.h"
#include "../include/dnscpp/watcher.h"
#include "../include/dnscpp/response.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>
#include <poll.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        event loop
 *  @param  handler     object that will receive all incoming responses
 *  @throws std::runtime_error
 */
Udps::Udps(Loop *loop, Handler *handler) : _loop(loop), _handler(handler)
{
    // trick to avoid a compiler warning
    Udp::Handler *udphandler = this;

    // create sockets
    _sockets.emplace_back(loop, udphandler);

    // set the first socket to use
    _current = _sockets.begin();
}

/**
 *  Update the number of sockets
 *  Watch out: it is only possible to _increase_ the number of sockets
 *  This is useful to spread out the load over multiple sockets (especially for programs with   
 *  many DNS lookups, where new lookups are started before previous lookups are completed)
 *  @param  value       new max value
 */
void Udps::sockets(size_t count)
{
    // trick to avoid a compiler warning
    Udp::Handler *udphandler = this;
    
    // create more sockets (note that we can only grow)
    for (size_t i = _sockets.size(); i < count; ++i) 
    {
        // create a new socket
        _sockets.emplace_back(_loop, udphandler);
        
        // give the socket the same settings as all other sockets
        _sockets.back().buffersize(_sockets.front().buffersize());
    }
}

/**
 *  Invoke callback handlers for buffered raw responses
 *  @param   watcher   to keep track if the parent object remains valid
 *  @param   maxcalls  the max number of callback handlers to invoke
 *  @return  number of callback handlers invoked
 */
size_t Udps::deliver(size_t maxcalls)
{
    // result variable
    size_t result = 0;

    // we are going to make a call to userspace, so we keep monitoring if `this` is not destructed
    Watcher watcher(this);

    // deliver from all sockets
    for (auto &socket : _sockets)
    {
        // tally up the numbers
        result += socket.deliver(&watcher, maxcalls);

        // It's possible that an onReceived handler will destruct the Context and consequently
        // this object too. In that case we have to be careful not to read/write any member
        // variables any longer.
        if (!watcher.valid()) break;
    }

    // return the number of buffered responses that were processed
    return result;
}

/**
 *  Send a query to a nameserver (+open the socket when needed)
 *  @param  ip      IP address of the nameserver
 *  @param  query   the query to send
 *  @return inbound
 */
Inbound *Udps::send(const Ip &ip, const Query &query)
{
    // If there is a socket with no subscribers: use that one + mark it as current
    for (auto iter = _sockets.begin(); iter != _sockets.end(); ++iter)
    {
        // continue if this one has subs
        if (iter->subscriberCount()) continue;

        // OK: send the query with this one
        Inbound *inbound = iter->send(ip, query);

        // mark it as current
        _current = iter;

        // done
        return inbound;
    }

    // If all the sockets already have subscribers: use the current
    return _current->send(ip, query);
}

/**
 *  Method that is called when an inbound socket is closed
 *  @param  udp     the reporting object
 */
void Udps::onClosed(Udp *udp)
{
    // @todo this method could be used to add more efficiency, for example
    // to mark the just closed socket as the current one, and avoid that every
    // send() call results in a loop over all sockets
}

/**
 *  End of namespace
 */
}
