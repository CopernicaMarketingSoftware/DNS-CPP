/**
 *  Extractor.cpp
 *
 *  Implementation file for the TLSA class
 *
 *  @author Raoul Wols <raoul.wols@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/tlsa.h"
#include <ostream>
#include <iomanip>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Write to a stream
 *  @param  stream
 *  @param  tlsa
 *  @return std::ostream
 */
std::ostream &operator<<(std::ostream &stream, const TLSA &tlsa)
{
    // print out the enums
    stream << (int)tlsa.usage() << " " << (int)tlsa.selector() << " " << (int)tlsa.hashing() << " ";

    // print the certificate association data as hex
    tlsa.printhex(stream, tlsa.data(), tlsa.size());

    // done
    return stream;
}

/**
 *  End of namespace
 */
}
