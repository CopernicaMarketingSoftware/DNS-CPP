/**
 *  Receiver.h
 * 
 *  Class that is responsible for reading data from a TCP socket
 *  until the full response has been received.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "../include/dnscpp/response.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Receiver : private Monitor
{
public:
    /**
     *  Interface to be implemented by the parent
     */
    class Handler
    {
    public:
        /**
         *  Method that is called when the response has been received
         *  @param  receiver        reporting object
         *  @param  response        the response data
         */
        virtual void onReceived(Receiver *receiver, const Response &response) = 0;
        
        /**
         *  Method that is called when the response was not received
         *  @param  receiver        reporting object
         */
        virtual void onFailure(Receiver *receiver) = 0;
    };
    
private:
    /**
     *  The TCP socket over which we're going to receive data
     *  @var Tcp
     */
    Tcp *_tcp;
    
    /**
     *  Buffer that is to be filled, the first two bytes contain the size of the message
     *  @var unsigned char[]
     */
    unsigned char *_buffer = nullptr;
    
    /**
     *  Size of the buffer
     *  @var size_t
     */
    size_t _size;
    
    /**
     *  Number of bytes that have already been received
     *  @var size_t
     */
    size_t _received;

    /**
     *  Handler that is notified when the data has been received
     *  @var Handler
     */
    Handler *_handler;
    
    /**
     *  Identifier of the monitor
     *  @var void*
     */
    void *_identifier;


    /**
     *  Size of the response -- this method only works if we have already received the frist two bytes
     *  @return uint16_t
     */
    uint16_t responsesize() const
    {
        // result variable
        uint16_t result;

        // get the first two bytes from the buffer
        memcpy(&result, _buffer, 2);
        
        // put the bytes in the right order
        return ntohs(result);
    }

    /**
     *  Number of bytes that we expect in the next read operation
     *  @return size_t
     */
    size_t expected() const
    {
        // if we have not yet received the first two bytes, we expect those first
        switch (_received) {
        case 0:     return 2;
        case 1:     return 1;
        default:    return (2 + responsesize()) - _received;
        }
    }
    
    /**
     *  Reallocate the buffer if it turns out that our buffer is too small for the expected response
     *  @return bool
     */
    bool reallocate()
    {
        // preferred buffer size
        size_t preferred = responsesize() + 2;
        
        // do nothing if the buffer is already good enough
        if (_size >= preferred) return true;
        
        // reallocate the buffer
        auto *newbuffer = (unsigned char *)realloc(_buffer, preferred);

        // leap out on failure
        if (newbuffer == nullptr) return false;

        // the buffer is bigger now
        _buffer = newbuffer;
        _size = preferred;
        
        // report result
        return true;
    }
    
    /**
     *  Method that is called by user space when the socket is readable
     */
    virtual void notify() override
    {
        // prevent exceptions (parsing the response could fail)
        try
        {
            // receive data from the socket
            auto result = _tcp->receive(_buffer + _received, expected());
            
            // do nothing if the operation is blocking
            if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            
            // if there is a failure we leap out
            if (result <= 0) throw false;
            
            // update the number of bytes received
            _received += result;
            
            // after we've received the first two bytes, we can reallocate the buffer so that it is of sufficient size
            if (_received == 2 && !reallocate()) throw false;
            
            // continue waiting if we have not yet received everything there is
            if (expected() > 0) return;
            
            // all data has been received, parse the response, and report to the handler
            _handler->onReceived(this, Response(_buffer + 2, _received - 2));
        }
        catch (...)
        {
            // we no longer need to monitor because this connection seems to be broken
            stop();
            
            // report this as error
            _handler->onFailure(this);
        }
    }


public:
    /**
     *  Constructor
     *  @param  tcp     socket that must already be connected
     *  @param  handler handler that will be notified when everything is ready
     */
    Receiver(Tcp *tcp, Handler *handler) : 
        _tcp(tcp),
        _buffer((unsigned char *)malloc(4096)),
        _size(4096),
        _received(0), 
        _handler(handler),
        _identifier(nullptr) {}
    
    /**
     *  Destructor
     */
    virtual ~Receiver()
    {
        // deallocate the buffer
        free(_buffer);
        
        // stop monitoring
        stop();
    }
    
    /**
     *  Stop the receiver
     *  @return bool
     */
    bool stop()
    {
        // not possible if not yet started
        if (_identifier == nullptr) return false;
        
        // stop monitoring
        _tcp->remove(_identifier, this);
        
        // we no longer need the identifier
        _identifier = nullptr;
        
        // done
        return true;
    }
    
    /**
     *  Start the receiver
     *  @return bool
     */
    bool start()
    {
        // not possible if already busy
        if (_identifier != nullptr) return false;
        
        // we are going to monitor the socket until it is readable
        _identifier = _tcp->monitor(1, this);
        
        // done
        return true;
    }
};    
    
/**
 *  End of namespace
 */
}

