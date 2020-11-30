/**
 *  Extractor.cpp
 * 
 *  Implementation file for the Extractor class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/extractor.h"
#include "canonicalizer.h"
#include <ostream>
#include <iomanip>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Write the rdata to a canonicalized form
 *  This method is used internally by the library to check DNSSEC signatures
 *  @param  canonicalizer
 *  @return bool
 */
bool Extractor::rdata(Canonicalizer &canonicalizer) const
{
    // default implementation writes the rdata as it is stored in the record
    return canonicalizer.add32(_record.size()) && canonicalizer.add(_record.data(), _record.size());
}

/**
 *  Print bytes as a stream of hexadecimal numbers
 *  @param stream
 *  @param bytes
 *  @param size
 *  @return same ostream
 */
std::ostream &Extractor::printhex(std::ostream &stream, const unsigned char *bytes, size_t size)
{
    // set up the state
    stream << std::hex << std::setfill('0') << std::setw(2);

    // std::copy(bytes, bytes + size, std::ostreambuf_iterator<std::ostream::char_type>{stream});

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
std::ostream &Extractor::printformattedtime(std::ostream &stream, const time_t time)
{
    // prepare a buffer
    char formatted[15];

    // format the time dig-style
    strftime(formatted, 15, "%Y%m%d%H%M%S", localtime(&time));

    // write it to the stream and return that same stream
    return stream << formatted;
}

/**
 *  End of namespace
 */
}
