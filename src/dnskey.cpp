/**
 *  DNSKEY.cpp
 * 
 *  Implementation file for the DNSKEY class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/dnskey.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  The key-tag. This is a sort of key-identifier that should match the
 *  keytag in the RRSIG record. If it does not match, it is pointless
 *  to even try validating the signature
 *  @return uint16_t
 */
uint16_t DNSKEY::keytag() const
{
    // the key and keysize
    auto keysize = _record.size();
    auto *key = _record.data();

    // for historical reasons, algorithm 1 (rsa/md5, which should not be used)
    // uses a different keytag algorithm, see https://tools.ietf.org/html/rfc4034#appendix-B.1
    if (algorithm() == Algorithm::RSAMD5)
    {
        // we expect more than four bytes
        if (keysize <= 4) return 0;
        
        // get the first two of the last three bytes
        return ns_get16(key + keysize - 3);
    }
    else
    {
        // result variable
        uint32_t result = 0;

        // check all characters in the key
        for (size_t i = 0; i < keysize; ++i)
        {
            // update result
           result += (i & 1) ? key[i] : key[i] << 8;
        }
        
        // update for last pos
        result += (result >> 16) & 0xFFFF;
        
        // expose last 16 bits
        return result & 0xFFFF;
    }
}    

/**
 *  End of namespace
 */
}

