/**
 *  A.cpp
 * 
 *  Simple test program to retrieve an A record
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include <ev.h>
#include <dnscpp.h>
#include <dnscpp/libev.h>
#include <iostream>

/**
 *  Our own handler
 */
class MyHandler : public DNS::Handler
{
public:
    /**
     *  Method that is called when a raw response is received
     *  @param  response        the received response
     */
    virtual void onReceived(const DNS::Response &response) override
    {
        std::cout << "got response" << std::endl;
    }
};

/**
 *  Main procedure
 *  @return int
 */
int main()
{
    // the event loop
    struct ev_loop *loop = EV_DEFAULT;
    
    // wrap the loop to make it accessible by dns-cpp
    DNS::LibEv myloop(loop);
    
    // create a dns context
    DNS::Context context(&myloop);

    // we need a handler
    MyHandler handler;

    // add a nameserver
    context.nameserver(DNS::Ip("8.8.8.8"));
    
    // do a query
    context.query("www.yahoo.com", ns_t_a, &handler);
    
    // run the event loop
    ev_run(loop, 0);
    
    // done
    return 0;
}
