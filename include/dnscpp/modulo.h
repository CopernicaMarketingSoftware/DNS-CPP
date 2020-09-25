/**
 *  Modulo.h
 *
 *  Inside DNSKEY records public keys are stored. When the key is
 *  of the form SHA1, SHA256 or SHA512, it contains an Exponent
 *  and a Modulo. These are big numbers that should be passed to
 *  a cryptographic library for further details.
 * 
 *  This is a utility class that can be used if you need to extract
 *  the modulo of a DNSKEY record
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */
 
/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Modulo : public Bignum
{
public:
    /**
     *  Constructor
     *  @param  rsasha      Object that already extracted RSA-SHAx properties
     */
    Modulo(const RSASHA &rsasha) : Bignum(rsasha.moduloData(), rsasha.moduloSize()) {}

    /**
     *  Constructor
     *  @param  dnskey      The key from which the data is to be extracted
     *  @throws std::runtime_error
     */
    Modulo(const DNSKEY &key) : Modulo(RSASHA(key)) {}
    
    /**
     *  Destructor
     */
    virtual ~Modulo() = default;
};
    
/**
 *  End of namepace
 */
}

