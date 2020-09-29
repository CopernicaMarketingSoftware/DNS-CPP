/**
 *  Message.cpp
 * 
 *  Implementation file for the Message class
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/message.h"
#include "../include/dnscpp/additional.h"
#include "../include/dnscpp/opt.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  The opcode, see arpa/nameser.h for supported opcodes
 *  @return ns_rcode
 */
ns_rcode Message::rcode() const 
{
    // get the code from the regular header
    auto rcode = ns_msg_getflag(_handle, ns_f_rcode);
    
    // if this is an EDNS message, there are extra rcode bits 
    for (size_t i = 0; i < additional(); ++i)
    {
        // prevent exceptions in case this is not an OPT record
        try
        {
            // get the additional record, and treat that as an OPT record
            OPT record(*this, Additional(*this, i));
        
            // if we're here we have indeed found the OPT record, and
            // should add the extra eight bits
            return ns_rcode(rcode | (record.rcode() << 4));
        }
        catch (...)
        {
            // OPT record could not be parsed, ignore this
        }
    }
    
    // no OPT record found, fallback to the original RCODE
    return ns_rcode(rcode);
}

/**
 *  Number of records of a certain type in a certain section
 *  @param  section     the type of section
 *  @param  type        the record type
 *  @param  dnsclass    the dnsclass
 *  @return uint16_t
 */
uint16_t Message::records(ns_sect section, uint16_t type, uint16_t dnsclass)
{
    // the result
    size_t result = 0;
    
    // the max number of records
    size_t max = records(section);
    
    // iterate over the records
    for (size_t i = 0; i < max; ++i)
    {
        // avoid exceptions
        try
        {
            // parse the record
            Record record(*this, section, i);
            
            // do we have a match
            if (record.type() == type && record.dnsclass() == dnsclass) result += 1;
        }
        catch (...) {}
    }
    
    // done
    return result;
}

/**
 *  End of namespace
 */
}
