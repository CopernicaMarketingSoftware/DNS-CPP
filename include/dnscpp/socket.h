/**
 *  Socket.h
 * 
 *  Common base class for a UDP and TCP socket
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <list>
#include "ip.h"
#include "inbound.h"
#include "watchable.h"

/**
 *  Begin of namespace
 */
namespace DNS {
    
/**
 *  Class definition
 */
class Socket : public Inbound, public Watchable
{
public:
    /**
     *  Interface to communicate with this object
     */
    class Handler
    {
    public:
        /**
         *  Method that is invoked when this object has buffered responses available
         *  @param  socket      the reporting object
         */
        virtual void onBuffered(Socket *socket) = 0;
    };
    
protected:
    /**
     *  Pointer to the handler
     *  @var Handler
     */
    Handler *_handler;

private:
    /**
     *  All the buffered responses that came in
     *  @var std::list
     */
    std::list<std::pair<Ip,std::vector<unsigned char>>> _responses;

protected:
    /**
     *  Constructor
     *  @param  handler
     */
    Socket(Handler *handler) : _handler(handler) {}
    
    /**
     *  Destructor
     */
    virtual ~Socket() = default;

    /**
     *  Add a message for delayed processing
     *  @param  addr    the address from which the message was received
     *  @param  buffer  the response buffer
     */
    void add(const sockaddr *addr, std::vector<unsigned char> &&buffer);
    void add(const Ip &addr, std::vector<unsigned char> &&buffer);

public:
    /**
     *  Invoke callback handlers for buffered raw responses
     *  @return  number of callback handlers invoked
     */
    virtual size_t deliver();

    /**
     *  Return true if there are buffered raw responses
     *  @return bool
     */
    bool buffered() const noexcept { return !_responses.empty(); }
};
    
/**
 *  End of namespace
 */
}

