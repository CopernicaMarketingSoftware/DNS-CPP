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
 *  @param  socketcount number of UDP sockets to keep open
 *  @throws std::runtime_error
 */
Udps::Udps(Loop *loop, Handler *handler, size_t socketcount) : _handler(handler)
{
    // we can't have zero sockets
    socketcount = std::max((size_t)1, socketcount);

    // trick to avoid a compiler warning
    Udp::Handler *udphandler = this;

    // create sockets
    for (size_t i = 0; i != socketcount; ++i) _sockets.emplace_back(loop, udphandler);

    // set the first socket to use
    _current = _sockets.begin();
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
 *  @param  buffersize
 *  @return inbound
 */
Inbound *Udps::send(const Ip &ip, const Query &query, int32_t buffersize)
{
    // If there is a socket with no subscribers: use that one + mark it as current
    for (auto iter = _sockets.begin(); iter != _sockets.end(); ++iter)
    {
        // continue if this one has subs
        if (iter->subscriberCount()) continue;

        // OK: send the query with this one
        Inbound *inbound = iter->send(ip, query, buffersize);

        // mark it as current
        _current = iter;

        // done
        return inbound;
    }

    // If all the sockets already have subscribers: use the current
    return _current->send(ip, query, buffersize);
}

/**
 *  Close all sockets
 *  @todo: this method should disappear
 */
void Udps::close()
{
    // close all sockets
    std::for_each(_sockets.begin(), _sockets.end(), std::mem_fun_ref(&Udp::close));
}

/**
 *  End of namespace
 */
}
