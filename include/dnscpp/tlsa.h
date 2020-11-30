/**
 *  TLSA.h
 *
 *  Helper class to parse the content in a TLSA record. A TLSA record holds
 *  information about whether/how connections to a domain are secured. It
 *  for example specifies whether the certificate that is used is self-signed,
 *  or whether it should come from a (specific) certificate-authority.
 *
 *  When you make a connection to mail.example.com, and you want to know 
 *  whether Copernica normally secures its connections, you can retrieve the
 *  TLS record from "_25._tcp.mail.example.com". This record holds the
 *  answer to your question.
 * 
 *  @author Raoul Wols <raoul.wols@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "extractor.h"
#include "response.h"
#include "type.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class TLSA : public Extractor
{
public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    TLSA(const Response &response, const Record &record) : Extractor(record, TYPE_TLSA, 3) {}

    /**
     *  The constructor
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    TLSA(const Record &record) : Extractor(record, TYPE_TLSA, 3) {}

    /**
     *  Destructor
     */
    virtual ~TLSA() = default;

    /**
     *  What sort of certificate is used? Note that certificates are chained, allowing
     *  people to buy a certificate from a Certificate Authority, and use that to create 
     *  sub-certificates for individual connections:
     * 
     *  A:   Root certificate > Certificate for example.com > Certificate for mail.example.com.
     * 
     *  Alternatively, people can create self-signed certificates, also with the
     *  option to chain them:
     * 
     *  B:   Self-signed certificate > Certificate for mydomain.com > Certificate for mail.mydomain.com.
     * 
     *  This chain can have any length, for self-signed certificates it is also
     *  possible that it is only one item long.
     * 
     *  In this 'usage' field it is stored what sort of certificate is in use ("A" or "B"),
     *  with the following values:
     * 
     *  0:  Variant "A" is used (thus there must be a chain back to a root-authority).
     *      And the "data" property must match with one of the certificates in the chain
     *      (it must thus match with "root" or "example.com" or "mail.example.com". This
     *      setting allows users to publish a DNS record, but use a derived certificate
     *      for the actual connection.
     * 
     *  1:  Variant "A" is used (thus there must be a chain back to a root-authority).
     *      And the "data" property must _exactly_ match with the "mail.example.com" certificate.
     *  
     *  2:  A self-signed certificate is used (so it is not necessary to check if the chain
     *      goes back to a root authority), and the actual certificate must match or be derived
     *      from that self-signed certificate.
     * 
     *  3:  A self-signed certigicate is used (so there is no need to check if the chain
     *      goes back to a root authority), and the actual certificate must exactly match.
     * 
     *  @return uint8_t
     */
    uint8_t usage() const { return _record.data()[0]; }
    
    /**
     *  The selector, meaning: does the "data" property hold the full certificate that should
     *  be matched, or just the public key of it? The public-key is normally also unique-enough and
     *  shorter to distribute. But in case a party has different certificates that happen to
     *  have the same public key (is this even possible / a one-in-billion-chance?) they can
     *  also publish the full certificate.
     * 
     *  0:  the full certificate must match
     *  1:  just the public key in DER format must match
     * 
     *  @return uint8_t
     */
    uint8_t selector() const { return _record.data()[1]; }
    
    /**
     *  Is the data in the record hashed or not? To reduce the size of the DNS record, it is
     *  possible for the publisher of the record to hash it, using one of the following algorithms:
     * 
     *  0:  No hashing was used
     *  1:  SHA256 hashing was used
     *  2:  SHA512 hashing was used
     * 
     *  @return uint8_t
     */
    uint8_t hashing() const { return _record.data()[2]; }
     
    /**
     *  The actual association data, this holds the certificate that should be used (possible hashed,
     *  possibly just the public key, see above)
     *  @return const unsigned char *
     */
    const unsigned char *data() const { return _record.data() + 3; }
    
    /**
     *  Size of the data
     *  @return size_t
     */
    size_t size() const { return _record.size() - 3; }

    /**
     *  Write to a stream
     *  @param  stream
     *  @param  tlsa
     *  @return std::ostream
     */
    friend std::ostream &operator<<(std::ostream &stream, const TLSA &tlsa);
};

/**
 *  End of namespace
 */
}
