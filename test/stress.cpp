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
#include <fstream>
#include <iomanip>
#include <sstream>

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
    size_t _servfails = 0;
    size_t _nxdomains = 0;
    
    /**
     *  Number of timeouts
     *  @var size_t
     */
    size_t _timeouts = 0;

    double percent(size_t count)
    {
        return 100.0 * ((double)count / _total);
    }

    std::string printStatistic(const char *name, size_t value)
    {
        std::ostringstream ss;
        ss << name << ": " << std::setw(7) << std::setprecision(4) << percent(value) << "%";
        return ss.str();
    }

    /**
     *  Show the status
     *  @param  operation       the operation that finished
     */
    void show(const DNS::Operation *operation)
    {
        if ((_success + _servfails + _nxdomains + _failures + _timeouts) % 100 != 0) return;

        std::cerr << printStatistic("success", _success)
            << ", " << printStatistic("servfails", _servfails)
            << ", " << printStatistic("nxdomains", _nxdomains)
            << ", " << printStatistic("failures", _failures)
            << ", " << printStatistic("timeouts", _timeouts)
            << '\n';
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
        // update counter
        switch (rcode)
        {
            case ns_r_servfail:
                _servfails += 1;
                break;
            case ns_r_nxdomain:
                _nxdomains += 1;
                break;
            default:
                _failures += 1;
                break;
        }

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

std::vector<std::string> readDomainList(const char *filename)
{
    std::vector<std::string> result;
    std::ifstream domainlist(filename);
    if (!domainlist) throw std::runtime_error("cannot open file");
    std::string line;
    while (std::getline(domainlist, line))
    {
        if (line.empty()) continue;
        result.push_back(line);
    }
    return result;
}

/**
 *  Main procedure
 *  @return int
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "give me a big list of domains, one per line, in a file\n";
        return EXIT_FAILURE;
    }

    // the event loop
    struct ev_loop *loop = EV_DEFAULT;

    // wrap the loop to make it accessible by dns-cpp
    DNS::LibEv myloop(loop);

    // create a dns context
    DNS::Context context(&myloop, true);

    context.buffersize(1024 * 1024); // size of the input buffer (high lowers risk of package loss)
    context.interval(3.0);           // number of seconds until the datagram is retried (possibly to next server) (this does not cancel previous requests)
    context.attempts(10);            // number of attempts until failure / number of datagrams to send at most
    context.capacity(20000);          // max number of simultaneous lookups per dns-context (high increases speed but also risk of package-loss)
    context.timeout(3.0);            // time to wait for a response after the _last_ attempt
    context.sockets(4);              // number of sockets

    // get domain list
    const auto domainlist = readDomainList(argv[1]);

    // handler for the lookups
    MyHandler handler(domainlist.size());

    for (const auto &domain : domainlist) {
        context.query(domain.c_str(), ns_t_mx, &handler);
    }

    // run the event loop
    ev_run(loop);
    
    // done
    return 0;
}

