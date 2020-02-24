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
        
        std::cout << "authentic: " << (int)response.authentic() << std::endl;
        std::cout << "checkingdisabled: " << (int)response.checkingdisabled() << std::endl;
        
        
        std::cout << "answers: " << response.answers() << std::endl;
        std::cout << "additional: " << response.additional() << std::endl;
        
        for (size_t i = 0; i < response.answers(); ++i)
        {
            DNS::Answer record(response, i);
            std::cout << "- record " << record.name() << " " << record.type() << " " << record.ttl() << std::endl;
            
            
            switch (record.type()) {
            case ns_t_a: {   
                DNS::A a(response, record);
                std::cout << "  -  " << a.ip() << std::endl;
                break;
            }
            case ns_t_rrsig: {
                DNS::RRSIG rrsig(response, record);
                std::cout << "  - " << rrsig.typeCovered() << " " << rrsig.algorithm() << " " << rrsig.validFrom() << " " << rrsig.validUntil() << " " << rrsig.signer() << std::endl; //" " << std::string((const char *)rrsig.signature(), rrsig.size()) << std::endl;
                break;
            }
            }
        }
        
        
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
    context.query("www.example.com", ns_t_a, &handler);
    
    // run the event loop
    ev_run(loop, 0);
    
    // done
    return 0;
}
