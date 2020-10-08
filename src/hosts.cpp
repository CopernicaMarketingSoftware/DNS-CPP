/**
 *  Hosts.cpp
 * 
 *  Implementation file for the Hosts class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include <fstream>
#include <vector>
#include <list>
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/hosts.h"
#include "../include/dnscpp/response.h"
#include "../include/dnscpp/handler.h"
#include "../include/dnscpp/question.h"
#include "../include/dnscpp/reverse.h"
#include "fakeresponse.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Helper method calculates the real line-size (minus whitespace)
 *  @param  line        the full line from the file
 *  @param  size        the size _with_ whitespace
 *  @return size_t      the size without whitespace
 */
static size_t linesize(const char *line, size_t size)
{
    // number of characters that can be trimmed
    size_t trim = 0;
    
    // check the trailing characters
    while (size > trim && isspace(line[size-1-trim])) ++trim;
    
    // done
    return size - trim;
}

/**
 *  Constructor
 *  @param  filename        the filename to parse
 *  @throws std::runtime_error
 */
Hosts::Hosts(const char *filename)
{
    // load initial file
    if (!load(filename)) throw std::runtime_error(std::string(filename) + ": failed to open file");
}

/**
 *  Load a certain file
 *  All lines in the file are merged with the lines already in memory
 *  @param  filename
 *  @return bool
 */
bool Hosts::load(const char *filename)
{
    // open the file for reading
    std::ifstream stream(filename);
    
    // file should be open by now
    if (!stream.is_open()) return false;
    
    // keep readling lines until the end
    while (!stream.eof())
    {
        // we are going to read lines, we need a local variable for that
        std::string line;
    
        // go read the line
        getline(stream, line);
        
        // remove trailing whitespace
        line.resize(linesize(line.data(), line.size()));
        
        // parse the line
        parse(line.data(), line.size());
    }
    
    // done
    return true;
}

/**
 *  Parse a line from the file
 *  @param  line        the line to parse
 *  @param  size        size of the line
 *  @return bool
 */
bool Hosts::parse(const char *line, size_t size)
{
    // skip empty lines and comments
    if (size == 0 || line[0] == '#') return true;
    
    // we are going to tokenize the string, for which we need a helper variable
    char *state = nullptr;

    // convert the buffer so that we can make changes to it
    std::vector<char> buffer(line, line + size + 1);
    
    // parse the first token holding the IP address
    auto *first = strtok_r(buffer.data(), "\t\r\n ", &state);
    
    // if there was not even an ip address
    if (first == nullptr) return true;
    
    // prevent exceptions
    try
    {
        // parse the ip (could throw)
        Ip ip(first);
        
        // now we are going to parse all hostnames
        while (true)
        {
            // get the next token, holding a hostname
            auto *token = strtok_r(nullptr, "\t\r\n ", &state);
            
            // stop when ready
            if (token == nullptr) return true;
            
            // turn the token into a string
            // @todo check if hostname is valid
            _hostnames.emplace_back(token);
            
            // get reference to the last item
            const auto &hostname = _hostnames.back();
            
            // insert into the maps
            _host2ip.emplace(std::make_pair(hostname.data(), ip));
            _ip2host.emplace(std::make_pair(ip, hostname.data()));
        }
        
        // success
        return true;
    }
    catch (...)
    {
        // invalid line address
        return false;
    }
}

/**
 *  Lookup an IP address given a hostname
 *  This method returns nullptr if there is no match
 *  @param  hostname        hostname for which we're looking for an ip
 *  @param  version         ip version (0 for nom matter)
 *  @return Ip
 */
const Ip *Hosts::lookup(const char *hostname, unsigned int version) const
{
    // look for a match
    const auto &range = _host2ip.equal_range(hostname);
    
    // go check the entire range
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        // get a reference to the ip
        const auto &ip = iter->second;
        
        // is this a match?
        if (version == 0 || ip.version() == version) return &ip;
    }
    
    // no match
    return nullptr;
}

/**
 *  Lookup a hostname given an IP address
 *  This method returns nullptr if there is no match
 *  @param  ip
 *  @return const char *
 */
const char *Hosts::lookup(const Ip &ip) const
{
    // look for a match
    const auto &iter = _ip2host.find(ip);
    
    // if there is no match
    if (iter == _ip2host.end()) return nullptr;
    
    // expose the ip
    return iter->second;
}

/**
 *  Notify a user-space handler about a certain hostname to ip combination
 *  @param  request        the original request
 *  @param  handler         user-space object that should be notified
 *  @param  operation       the operation-pointer that should be passed
 *  @return bool
 */
bool Hosts::notify(const Request &request, Handler *handler, const Operation *operation) const
{
    // extract the question
    Question question(request);
    
    // construct the fake response message that we will pass to user-space
    FakeResponse response(request, question);

    // is this a reverse lookup?
    if (question.type() == TYPE_PTR)
    {
        // parse the reverse name
        Reverse reverse(question.name());
        
        // get the associated hostname
        auto *hostname = lookup(reverse.ip());
        
        // add to the response message
        if (hostname) response.append(question.name(), hostname);
    }
    else
    {
        // do the lookup of ip-addresses of the requested host
        const auto &range = _host2ip.equal_range(question.name());
    
        // look for matches
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            // get a reference to the ip
            const auto &ip = iter->second;
            
            // is this a match?
            if (ip.version() == 4 && question.type() != TYPE_A) continue;
            if (ip.version() == 6 && question.type() != TYPE_AAAA) continue;
            
            // we have a matc
            response.append(question.name(), ip);
        }
    }
    
    // send back the answer
    handler->onReceived(operation, Response(response.data(), response.size()));
    
    // done
    return true;
}

/**
 *  End of namespace
 */
}

