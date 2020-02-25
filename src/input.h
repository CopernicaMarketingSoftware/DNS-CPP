/**
 *  Input.h
 * 
 *  Class that represents the input that is given to the signing
 *  algorithm. The signature that is stored in a RRSIG record is the
 *  cryptographic signature of a certain input-string. To reconstruct 
 *  this input, you can use this Input class.
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "canonicalizer.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Input : private Canonicalizer
{
private:
    /**
     *  The original RRSIG record that holds the signature
     *  @var RRSIG
     */
    const RRSIG &_rrsig;
    
    /**
     *  Canonicalized name for the RRSIG
     *  @var Name
     */
    Name _owner;

public:
    /**
     *  Constructor
     *  To reconstruct the input string, we also need information from
     *  the original signature, so you pass that to the constructor
     *  @param  rrsig
     *  @throws std::runtime_error
     */
    Input(const RRSIG &rrsig) : _rrsig(rrsig), _owner(_rrsig.name())
    {
        // add all properties of the signature
        add16(_rrsig.typeCovered());
        add8(_rrsig.algorithm());
        add8(_rrsig.labels());
        add32(_rrsig.originalTtl());
        add32(_rrsig.validUntil());
        add32(_rrsig.validFrom());
        add8(_rrsig.keytag());
        
        // canonicalize the signer name too (this is the last input as
        // the signature data itselves does not have to be included)
        // @todo should we include wildcards here?
        _owner.canonicalize(*this);
    }

    /**
     *  Destructor
     */
    virtual ~Input() = default;
    
    /**
     *  Method to add a record to the input
     *  @param  record
     *  @return bool
     */
    bool add(const Extractor &record)
    {
        // we only accept records of a similar type
        if (record.type() != _rrsig.typeCovered()) return false;
        
        // class must be the same
        if (record.dnsclass() != _rrsig.dnsclass()) return false;
        
        // ttl's must also match
        if (record.ttl() != _rrsig.ttl()) return false;
        
        // get the owner name
        Name owner(record.name());
        
        // owner names must match
        if (_owner != owner) return false;
        
        // get the old size
        size_t oldsize = size();
        
        // pseudo-loop to allow breaking out in case of an error
        do
        {
            // stuff can be added
            // @todo wildcard support / use _rrsig.labels()
            if (!owner.canonicalize(*this)) break;
            
            // add type, class and ttl
            if (!add16(record.type())) break;
            if (!add16(record.dnsclass())) break;
            if (!add32(_rrsig.originalTtl())) break;
            
            // canonicaize the rdata
            if (!record.rdata(*this)) break;
            
            // all was ok
            return true;
        }
        while (false);
        
        // record could not be added, restore the original buffer
        restore(oldsize);
        
        // report error
        return false;
    }
};
    
/**
 *  End of namespace
 */
}
