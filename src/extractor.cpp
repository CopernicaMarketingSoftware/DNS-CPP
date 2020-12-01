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
 *  End of namespace
 */
}
