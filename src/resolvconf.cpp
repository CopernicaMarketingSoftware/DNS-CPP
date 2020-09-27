/**
 *  ResolvConf.cpp
 * 
 *  Implementation-file for the resolv.conf parser
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */
 
/**
 *  Dependencies
 */
#include "../include/dnscpp/ip.h"
#include "../include/dnscpp/resolvconf.h"
#include <ctype.h>
#include <fstream>

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
    return trim;
}
    
/**
 *  Check if a line starts with a certain word
 *  @param  line        the line to parse (must already be trimmed)
 *  @param  size        size of the line
 *  @param  required    the word to start with
 *  @return size_t      number of initial bytes that can be stripped
 */
static size_t check(const char *line, size_t size, const char *required)
{
    // line of the word to check
    size_t skip = strlen(required);
    
    // check for the beginning
    if (strncasecmp(line, required, skip) != 0) return 0;
    
    // check the amount of whitespace
    size_t whitespace = 0;
    
    // how many chars can be skipped
    while (size > skip+whitespace && isspace(line[skip+whitespace])) ++whitespace;
    
    // if there was no whitespace at all the option does not have a value, or it
    // is not skipped with whitespace from the value, we treat this as a no-match
    if (whitespace == 0) return 0;
    
    // we now know the total size
    return skip + whitespace;
}

/**
 *  Constructor
 *  @param  filename
 *  @throws std::runtime_error
 */
ResolvConf::ResolvConf(const char *filename)
{
    // open the file for reading
    std::ifstream stream(filename);
    
    // file should be open by now
    if (!stream.is_open()) throw std::runtime_error(std::string(filename) + ": failed to open file");
    
    // catch exceptions to reformat them
    try
    {
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
    }
    catch (const std::runtime_error &error)
    {
        // reformat the error
        throw std::runtime_error(std::string(filename) + ": " + error.what());
    }
}

/**
 *  Helper method to parse lines
 *  @param  line        the line to parse (must already be trimmed)
 *  @param  size        size of the line
 *  @throws std::runtime_error
 */
void ResolvConf::parse(const char *line, size_t size)
{
    // skip empty lines or lines that are commented out
    if (size == 0 || line[0] == ';' || line[0] == '#') return;
    
    // helper variable
    size_t skip = 0;
    
    // check the instruction
    if ((skip = check(line, size, "nameserver")) > 0) nameserver(line+skip, size-skip);
    if ((skip = check(line, size, "options")) > 0) options(line+skip, size-skip);
    if ((skip = check(line, size, "domain")) > 0) domain(line+skip, size-skip);
    if ((skip = check(line, size, "search")) > 0) search(line+skip, size-skip);
    
    // invalid line
    throw std::runtime_error(std::string("unrecognized: ") + line);
}
    
/**
 *  Parse a line holding a nameserver
 *  @param  line        the value to parse
 *  @param  size        size of the line
 *  @throws std::runtime_error
 */
void ResolvConf::nameserver(const char *line, size_t size)
{
    // add a nameserver
    _nameservers.emplace_back(line);
}

/**
 *  Add the local domain
 *  @param  line        the value to parse
 *  @param  size        size of the line
 *  @throws std::runtime_error
 */
void ResolvConf::domain(const char *line, size_t size)
{
    // report error
    throw std::runtime_error(std::string("not implemented: ") + line);
}

/**
 *  Add a search path
 *  @param  line        the value to parse
 *  @param  size        size of the line
 *  @throws std::runtime_error
 */
void ResolvConf::search(const char *line, size_t size)
{
    // report error
    throw std::runtime_error(std::string("not implemented: ") + line);
}

/**
 *  Add an option
 *  @param  line        the value to parse
 *  @param  size        size of the line
 *  @throws std::runtime_error
 */
void ResolvConf::options(const char *line, size_t size)
{
    // report error
    throw std::runtime_error(std::string("not implemented: ") + line);
}
  
/**
 *  End of namespace
 */
}

