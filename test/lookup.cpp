/**
 *  Lookup.cpp
 * 
 *  Test program for DNS lookups
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
#include <iomanip>

/**
 *  Helper function to convert type
 *  @param  input
 *  @return int
 *  @throws std::runtime_error
 */
static ns_type convert(const char *type)
{
    // check all types
    if (strcasecmp(type, "a")       == 0) return ns_t_a;
    if (strcasecmp(type, "aaaa")    == 0) return ns_t_aaaa;
    if (strcasecmp(type, "mx")      == 0) return ns_t_mx;
    if (strcasecmp(type, "txt")     == 0) return ns_t_txt;
    if (strcasecmp(type, "cname")   == 0) return ns_t_cname;
    if (strcasecmp(type, "ptr")     == 0) return ns_t_ptr;
    if (strcasecmp(type, "caa")     == 0) return ns_t_caa;
    if (strcasecmp(type, "ns")      == 0) return ns_t_ns;
    if (strcasecmp(type, "ptr")     == 0) return ns_t_ptr;
    if (strcasecmp(type, "tlsa")    == 0) return ns_t_tlsa;
    
    // invalid type
    throw std::runtime_error(std::string("unknown record type ") + type);
}

/**
 *  Print bytes as a stream of hexadecimal numbers
 *  @param stream
 *  @param bytes
 *  @param size
 *  @return same ostream
 */
static std::ostream &printhex(std::ostream &stream, const unsigned char *bytes, size_t size)
{
    // set up the state
    stream << std::hex << std::setfill('0') << std::setw(2);

    // print each character
    for (size_t i = 0; i < size; ++i) stream << (int)bytes[i];

    // restore state -- this is a bit flakey because we don't actually store the
    // previous state. @todo: revisit in the future
    return stream << std::dec << std::setfill(' ') << std::setw(0);
}

/**
 *  Print formatted time a-la `dig`-style
 *  @param stream
 *  @param time
 *  @return same ostream
 */
static std::ostream &printformattedtime(std::ostream &stream, const time_t time)
{
    // prepare a buffer
    char formatted[15];

    // format the time dig-style
    strftime(formatted, 15, "%Y%m%d%H%M%S", localtime(&time));

    // write it to the stream and return that same stream
    return stream << formatted;
}

/**
 *  Write to a stream
 *  @param  stream
 *  @param  tlsa
 *  @return std::ostream
 */
static std::ostream &operator<<(std::ostream &stream, const DNS::TLSA &tlsa)
{
    // print out the enums
    stream << (int)tlsa.usage() << " " << (int)tlsa.selector() << " " << (int)tlsa.hashing() << " ";

    // print the certificate association data as hex
    return printhex(stream, tlsa.data(), tlsa.size());
}

/**
 *  Write to a stream
 *  @param  stream
 *  @param  signature
 *  @return std::ostream
 */
static std::ostream &operator<<(std::ostream &stream, const DNS::RRSIG &sig)
{
    // print the basic properties
    stream
        << (int)sig.typeCovered()  << " "
        << (int)sig.algorithm()  << " "
        << (int)sig.labels()  << " "
        << (int)sig.originalTtl() << " ";

    // print the timestamps dig-style
    printformattedtime(stream, sig.validUntil()) << " ";
    printformattedtime(stream, sig.validFrom()) << " ";

    // print the rest of the properties
    stream << (int)sig.keytag() << " " << sig.signer() << " ";

    // print the signature
    // it should be base64-encoded a-la `dig`-style, but we don't have such a
    // function in the library
    return stream << "<signature of length " << sig.size() << ">";
}

/**
 *  Class to handle responses
 */
class MyHandler : public DNS::Handler
{
public:
    /**
     *  Method that is called when an operation times out.
     *  @param  operation       the operation that timed out
     */
    virtual void onTimeout(const DNS::Operation *operation) override
    {
        // report the error
        std::cout << "timeout" << std::endl;
        
        // stop the event loop
        ev_break(EV_DEFAULT, EVBREAK_ONE);
    }
    
    /**
     *  Method that is called when a raw response is received.
     *  @param  operation       the operation that finished
     *  @param  response        the received response
     */
    virtual void onReceived(const DNS::Operation *operation, const DNS::Response &response) override
    {
        // opcode status and id
        std::cout << ";; Opcode: " << response.opcode() << ", status: " << response.rcode() << ", id: " << response.id() << std::endl;
        
        // flags, number of sections
        std::cout << ";; Flags: " 
            << (response.question() ? "qr " : "")
            << (response.authoratative() ? "aa " : "")
            << (response.truncated() ? "tc " : "")
            << (response.recursiondesired() ? "rd " : "")
            << (response.recursionavailable() ? "ra " : "")
            << (response.authentic() ? "ad" : "")
            << (response.checkingdisabled() ? "cd": "")
            << "; QUERY: " << response.records(ns_s_qd)
            << ", ANSWER: " << response.records(ns_s_an)
            << ", AUTHORITY: " << response.records(ns_s_ns)
            << ", ADDITIONAL: " << response.records(ns_s_ar) << std::endl;
            
        // print the sections
        printsection(response, "QUESTION", ns_s_qd);
        printsection(response, "ANSWER", ns_s_an);
        printsection(response, "AUTHORITY", ns_s_ns);

        // stop the event loop
        ev_break(EV_DEFAULT, EVBREAK_ONE);
    }

private:
    /**
     *  Helper method to print a section    
     *  @param  response
     *  @param  name
     *  @param  section
     */
    static void printsection(const DNS::Response &response, const char *name, ns_sect section)
    {
        // number of records
        size_t count = response.records(section);
        
        // do nothing if section is empty
        if (count == 0) return;
        
        // output header
        std::cout << ";; " << name << " SECTION" << std::endl;
        
        // parse all records
        for (size_t i = 0; i < count; ++i)
        {
            // the record
            DNS::Record record(response, section, i);
            
            // output something
            std::cout << record.name() << "\tttl:" << record.ttl() << "\tclass:" << record.dnsclass() << "\ttype:" << record.type() << "\t";
            
            try
            {
                // check the type
                if (section != ns_s_qd) switch (record.type()) {
                case ns_t_a:        std::cout << DNS::A(response, record).ip(); break;
                case ns_t_aaaa:     std::cout << DNS::AAAA(response, record).ip(); break;
                case ns_t_mx:       std::cout << DNS::MX(response, record).priority() << " " << DNS::MX(response, record).hostname(); break;
                case ns_t_cname:    std::cout << DNS::CNAME(response, record).target(); break;
                case ns_t_txt:      std::cout << DNS::TXT(response, record).data(); break;
                case ns_t_ns:       std::cout << DNS::NS(response, record).nameserver(); break;
                case ns_t_ptr:      std::cout << DNS::PTR(response, record).target(); break;
                case ns_t_soa:      { DNS::SOA soa(response, record); std::cout << soa.nameserver() << " " << soa.email() << " " << soa.serial() << " " << soa.interval() << " " << soa.retry() << " " << soa.expire() << " " << soa.minimum(); } break;
                case ns_t_tlsa:     std::cout << DNS::TLSA(response, record); break;
                case ns_t_rrsig:    std::cout << DNS::RRSIG(response, record); break;
                default:            std::cout << "unknown"; break;
                }
            
            }
            catch (const std::runtime_error &error)
            {
                std::cout << "parse error " << error.what();
            }
            
            // newline
            std::cout << std::endl;
        }
        
        // mark end of section
        std::cout << std::endl;
    }
};

/** Main procedure
 *  @param  argc
 *  @param  argv
 *  @return int
 */
int main(int argc, const char *argv[])
{
    // avoid exceptions
    try
    {
        // the event loop
        struct ev_loop *loop = EV_DEFAULT;
    
        // parse the /etc/resolv.conf file
        DNS::ResolvConf config;
    
        // wrap the loop to make it accessible by dns-cpp
        DNS::LibEv myloop(loop);
        
        // create a dns context
        DNS::Context context(&myloop, config);
        
        // check usage
        if (argc != 3) throw std::runtime_error(std::string("usage: ") + argv[0] + " type value");
        
        // the type
        ns_type type = convert(argv[1]);
        const char *value = argv[2];

        // we are DNSSEC-aware
        context.dnssec(true);
        
        // object that will handle the call
        MyHandler handler;
        
        // do the lookup
        auto *operation = context.query(value, type, &handler);
        
        // check if the query could not even be started
        if (operation == nullptr) throw std::runtime_error(std::string("failed to query ") + value);
        
        // run the event loop
        ev_run(loop);
        
        // success
        return 0;
    }
    catch (const std::runtime_error &error)
    {
        // report error
        std::cerr << error.what() << std::endl;
        
        // exit code
        return -1;
    }
}

