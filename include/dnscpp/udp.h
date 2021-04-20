#pragma once

#include "monitor.h"
#include "inbound.h"
#include "socket.h"
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
class Udp : public Socket, private Monitor
{
private:
    /**
     *  The main event loop
     *  @var Loop*
     */
    Loop *_loop;

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
     *  Method that is called when there are no more subscribers, and that 
     *  is implemented in the derived classes. Watch out: this method can be called
     *  in the middle of loop through sockets so the implementation must be careful.
     */
    virtual void reset() override;

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

    /**
     *  Close the socket
     *  @return bool
     */
    void close();

public:
    /**
     *  Constructor does nothing but store a pointer to a handler object.
     *  Sockets are opened lazily
     *  @param  loop        the event loop
     *  @param  handler     parent object that is notified in case of relevant events
     */
    Udp(Loop *loop, Socket::Handler *handler);

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
     *  Install a new buffersize
     *  @param  size        size of the new buffer
     */
    void buffersize(size_t size) { _buffersize = size; }

    /**
     *  Expose the used buffersize
     *  @return size_t
     */
    size_t buffersize() const { return _buffersize; }
};

/**
 *  End namespace
 */
}
