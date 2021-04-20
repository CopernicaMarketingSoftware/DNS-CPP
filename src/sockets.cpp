/**
 *  Sockets.cpp
 * 
 *  Implementation file for the Sockets class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/sockets.h"
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
#include "connector.h"

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
Sockets::Sockets(Loop *loop, Handler *handler) : _loop(loop), _handler(handler)
{
    // trick to avoid a compiler warning
    Udp::Handler *udphandler = this;

    // create a first UDP socket
    _udps.emplace_back(loop, udphandler);

    // set the first socket to use
    _current = _udps.begin();
}

/**
 *  Update the number of sockets
 *  Watch out: it is only possible to _increase_ the number of sockets
 *  This is useful to spread out the load over multiple sockets (especially for programs with   
 *  many DNS lookups, where new lookups are started before previous lookups are completed)
 *  @param  value       new max value
 */
void Sockets::sockets(size_t count)
{
    // trick to avoid a compiler warning
    Udp::Handler *udphandler = this;
    
    // create more sockets (note that we can only grow)
    for (size_t i = _udps.size(); i < count; ++i) 
    {
        // create a new socket
        _udps.emplace_back(_loop, udphandler);
        
        // give the socket the same settings as all other sockets
        _udps.back().buffersize(_udps.front().buffersize());
    }
}

/**
 *  Invoke callback handlers for buffered raw responses
 *  @param   watcher   to keep track if the parent object remains valid
 *  @param   maxcalls  the max number of callback handlers to invoke
 *  @return  number of callback handlers invoked
 */
size_t Sockets::deliver(size_t maxcalls)
{
    // result variable
    size_t result = 0;

    // we are going to make a call to userspace, so we keep monitoring if `this` is not destructed
    Watcher watcher(this);

    // deliver the buffer from all udp sockets
    for (auto &socket : _udps)
    {
        // pass the buffered responses to the lookup objects
        result += socket.deliver(maxcalls);
        
        // update number of calls
        maxcalls -= result;

        // the call to userspace may have destructed this object
        if (!watcher.valid()) return result;
        
        // leap out if we have made all the calls back to userspace
        if (maxcalls == 0) return result;
    }
    
    // if there are no more tcp connections, we're done
    if (_tcps.empty()) return result;
    
    // when responses from tcp sockets are delivered, it is possible that at the same
    // time those sockets are removed from the underlying array (by the onUnused() method),
    // to ensure that we can use a simple iterator, we make a copy of the sockets
    auto sockets = _tcps;
    
    // deliver the buffer from all tcp sockets
    for (auto &socket : sockets)
    {
        // pass the buffered responses to the lookup objects
        result += socket->deliver(maxcalls);
        
        // update number of calls
        maxcalls -= result;

        // the call to userspace may have destructed this object
        if (!watcher.valid()) return result;
        
        // leap out if we have made all the calls back to userspace
        if (maxcalls == 0) return result;
    }
    
    // return the number of buffered responses that were processed
    return result;
}

/**
 *  Method that is called when a TCP socket becomes unused
 *  @param  socket      the reporting socket
 */
void Sockets::onUnused(Tcp *socket)
{
    // when a tcp socket is no longer in use (nobody is expecting any answers from it) we close it right away
    _tcps.erase(std::remove_if(_tcps.begin(), _tcps.end(), [socket](const std::shared_ptr<Tcp> &tcp) -> bool {
        return socket == tcp.get();
    }), _tcps.end());
}

/**
 *  Send a query to a nameserver (+open the socket when needed)
 *  @param  ip      IP address of the nameserver
 *  @param  query   the query to send
 *  @return inbound
 */
Inbound *Sockets::datagram(const Ip &ip, const Query &query)
{
    // We have a simple algorithm to spread out the load over different sockets, so that we 
    // sometimes switch port-numbers for outgoing queries, which makes the system safer: when
    // all the sockets are in use (expect one or more responses), we use one socket for all
    // subsequent queries to allow the other sockets to process all their responses, and then
    // be closed to cycle their port number. But we first check if there is already an unused
    // sockets, because in that case we can use that for sending out a query with a fresh port
    for (auto iter = _udps.begin(); iter != _udps.end(); ++iter)
    {
        // continue if this one has subscribers / is already in use
        if (iter->subscribers() > 0) continue;

        // OK: send the query with this one
        Inbound *inbound = iter->send(ip, query);

        // mark it as current so that we use this socket for subsequent queries in case all
        // the sockets are in use by now
        _current = iter;

        // done
        return inbound;
    }

    // this is the situation that all sockets have subscribers and we start using a fixed
    // socket to allow the others to catch up
    return _current->send(ip, query);
}

/**
 *  Connect to a certain IP
 *  @param  ip          IP address of the target nameservers
 *  @param  connector   the object interested in the connection
 *  @return bool
 */
bool Sockets::connect(const Ip &ip, Connector *connector)
{
    // check if we already have a connection to this ip
    for (auto &tcp : _tcps)
    {
        // is this the one?
        if (tcp->ip() != ip) continue;
        
        // subscribe to the connection, so that it will be notified when ready
        if (tcp->subscribe(connector)) return true;
    }
    
    // avoid exceptions to bubble up
    try
    {
        // get the socket-handler
        Socket::Handler *handler = this;
        
        // create a brand new connection
        auto tcp = std::make_shared<Tcp>(_loop, ip, handler);
        
        // add this to the list
        _tcps.push_back(tcp);
        
        // subscribe to the connection, so that it will be notified when ready (this
        // always returns true because we just created the object and it cannot yet
        // be in a failed state)
        return tcp->subscribe(connector);
    }
    catch (...)
    {
        // failure
        return false;
    }
}

/**
 *  End of namespace
 */
}
