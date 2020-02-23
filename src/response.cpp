/**
 *  Response.cpp
 * 
 *  Implementation file for the response class
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/response.h"
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
ns_rcode Response::rcode() const 
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
 *  End of namespace
 */
}
