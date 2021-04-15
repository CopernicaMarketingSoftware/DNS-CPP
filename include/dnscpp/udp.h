#pragma once

#include "monitor.h"
#include "inbound.h"
#include <list>

/**
 *  Begin namespace
 */
namespace DNS {

class Loop;
class Query;
class Watcher;

/**
 *  Class declaration
 */
class Udp : public Inbound, private Monitor
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
         *  @param  udp     the reporting object
         */
        virtual void onBuffered(Udp *udp) = 0;
        
        /**
         *  Method that is called when this socket has closed (and is no longer in use
         *  @param  udp     the reporting object
         */
        virtual void onClosed(Udp *udp) = 0;
    };

private:
    /**
     *  The main event loop
     *  @var Loop*
     */
    Loop *_loop;

    /**
     *  Pointer to a handler object
     *  @var Udp*
     */
    Handler *_handler = nullptr;

    /**
     *  User space identifier of this monitor
     *  @var void *
     */
    void *_identifier = nullptr;

    /**
     *  The filedescriptor of the socket
     *  @var int
     */
    int _fd = -1;

    /**
     *  Buffersize for inbound sockets
     *  @var size_t
     */
    size_t _buffersize;

    /**
     *  All the buffered responses that came in
     *  @var std::list
     */
    std::list<std::pair<Ip,std::basic_string<unsigned char>>> _responses;

    /**
     *  Helper method to set an integer socket option
     *  @param  optname
     *  @param  optval
     */
    int setintopt(int optname, int32_t optval);

    /**
     *  Check whether this socket has a valid file descriptor
     *  @return bool
     */
    bool valid() const noexcept { return _fd > 0; }

    /**
     *  Implements the Monitor interface
     */
    void notify() override;

    /**
     *  Send a query to a certain nameserver
     *  @param  address     target address
     *  @param  size        size of the address
     *  @param  query       query to send
     *  @return bool
     */
    bool send(const struct sockaddr *address, size_t size, const Query &query);

    /**
     *  Open the socket
     *  @param  version     IPv4 or IPv6. You must ensure this stays consistent over multiple requests (@todo)
     *  @return bool
     */
    bool open(int version);

public:
    /**
     *  Constructor does nothing but store a pointer to a handler object.
     *  Sockets are opened lazily
     *  @param  loop        the event loop
     *  @param  handler     parent object that is notified in case of relevant events
     */
    Udp(Loop *loop, Handler *handler);

    /**
     *  Closes the file descriptor
     */
    virtual ~Udp();

    /**
     *  Send a query over this socket
     *  @param  ip IP address to send to. The port is always assumed to be 53.
     *  @param  query  The query
     *  @return this, or nullptr if something went wrong
     */
    Inbound *send(const Ip &ip, const Query &query);

    /**
     *  Close the socket
     *  @return bool
     */
    void close() override;

    /**
     *  Install a new buffersize
     *  @param  size        size of the new buffer
     */
    void buffersize(size_t size) { _buffersize = size; }

    /**
     *  Invoke callback handlers for buffered raw responses
     *  @param   watcher   to keep track if the parent object remains valid
     *  @param   maxcalls  the max number of callback handlers to invoke
     *  @return  number of callback handlers invoked
     */
    size_t deliver(Watcher *watcher, size_t maxcalls);

    /**
     *  Return true if there are buffered raw responses
     *  @return bool
     */
    bool buffered() const noexcept { return !_responses.empty(); }
};

/**
 *  End namespace
 */
}
