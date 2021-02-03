/**
 *  Stress.cpp
 * 
 *  Program to do a crazy amount of lookups to see if the library can
 *  keep up with this.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include <dnscpp.h>
#include <iostream>
#include <ev.h>
#include <dnscpp/libev.h>
#include <math.h>

/**
 *  Class creates a test-domain
 */
class TestDomain
{
private:
    /**
     *  The actual domain
     *  @var std::string
     */
    std::string _domain;
    
    /**
     *  Size of the domain (without the postfix)
     *  @return size_t
     */
    size_t size() const
    {
        // find the dot
        return _domain.find('.');
    }
    
    /**
     *  Increment the domain on a certain position
     *  @param  pos
     */
    void increment(int pos)
    {
        // check if we can increment this position
        if (_domain[pos] != 'z') { _domain[pos] += 1; return; }
        
        // we go back to the begin
        _domain[pos] = 'a';
        
        // increment the next
        if (pos > 0) increment(pos-1);
        
        // cycle / start over
        else _domain = "aa.com";
    }


public:
    /**
     *  Constructor
     *  @param  size
     */
    TestDomain(size_t size = 3) : _domain(size, 'a') 
    {
        // add postfix
        _domain.append(".com");
    }
    
    /**
     *  Expose the domain
     *  @return const char *
     */
    operator const char * () const { return _domain.data(); }
    
    /**
     *  Increment the domain (go to the next)
     */
    void increment() { increment(size() - 1); }
    
    /**
     *  Number of combinations
     *  @return size_t
     */
    size_t combinations() const { return pow(26, size()); }
    
    /**
     *  Is it at the beginning?
     *  @return bool
     */
    bool first() const
    {
        // check all chars
        for (size_t i = 0; true; ++i)
        {
            // check the char
            switch (_domain[i]) {
            case '.':   return true;
            case 'a':   break;
            default:    return false;
            }
        }
        
        // unreachable
        return false;
    }
};

/**
 *  The handler class
 */
class MyHandler : public DNS::Handler
{
private:
    /**
     *  Number of domains that are checked
     *  @var size_t
     */
    size_t _total = 0;

    /**
     *  Number of successful lookups
     *  @var size_t
     */
    size_t _success = 0;
    
    /**
     *  Number of errors
     *  @var size_t
     */
    size_t _failures = 0;
    
    /**
     *  Number of timeouts
     *  @var size_t
     */
    size_t _timeouts = 0;

    /**
     *  Show the status
     *  @param  operation       the operation that finished
     */
    void show(const DNS::Operation *operation)
    {
        // parse the original request
        DNS::Request request(operation);
        DNS::Question question(request);
        
        // show result
        std::cout << _total << " " << _success << " " << _failures << " " << _timeouts << " " << (_success + _failures + _timeouts) << " (" << question.name() << ")" << std::endl;
    }

    /**
     *  Method that is called when a valid, successful, response was received.
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onResolved(const DNS::Operation *operation, const DNS::Response &response) override
    {
        // update counter
        _success += 1;
        
        // show what is going on
        show(operation);
    }

    /**
     *  Method that is called when a query could not be processed or answered. 
     *  @param  operation       the operation that finished
     *  @param  rcode           the received rcode
     */
    virtual void onFailure(const DNS::Operation *operation, int rcode) override
    {
//        DNS::Request request(operation);
//        DNS::Question question(request);
//        
//        std::cout << question.name() << std::endl;


        // update counter
        _failures += 1;

        // show what is going on
        show(operation);
    }

    /**
     *  Method that is called when an operation times out.
     *  @param  operation       the operation that timed out
     */
    virtual void onTimeout(const DNS::Operation *operation) override
    {
        // update counter
        _timeouts += 1;

        // show what is going on
        show(operation);
    }

public:
    /**
     *  Constructor
     *  @param  total
     */
    MyHandler(size_t total) : _total(total) {}
    
    
};


/**
 *  Main procedure
 *  @return int
 */
int main()
{
//    srand(time(nullptr));
    
    // the event loop
    struct ev_loop *loop = EV_DEFAULT;

    // wrap the loop to make it accessible by dns-cpp
    DNS::LibEv myloop(loop);
    
    // create a dns context
    DNS::Context context(&myloop);

    context.buffersize(4 * 1024 * 1024);        // size of the input buffer (high lowers risk of package loss)
    context.interval(2.0);                      // number of seconds until the datagram is retried (possibly to next server) (this does not cancel previous requests)
    context.attempts(50);                       // number of attempts until failure / number of datagrams to send at most
    context.capacity(1000);                     // max number of simultaneous lookups per dns-context (high increases speed but also risk of package-loss)
    context.timeout(10.0);                      // time to wait for a response after the _last_ attempt

    // start with a domain
    TestDomain domain(4);
    
    // handler for the lookups
    MyHandler handler(domain.combinations());
    
    size_t i = 0;
    
    // show all the domains
    do
    {
        if (++i > 20000) break;
        
        // do a lookup
        context.query(domain, ns_t_mx, &handler);
        
        // go to next
        domain.increment();
    }
    while (!domain.first());
    
    // run the event loop
    ev_run(loop);
    
    // done
    return 0;
}

