/**
 *  TLSA.h
 *
 *  Helper class to parse the content in a TLSA record
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
     *  Specifies how to verify the certificate.
     */
    enum class CertificateUsage : uint8_t
    {
        // The certificate provided when establishing TLS must be issued by the
        // listed root-CA or one of its intermediate CAs, with a valid
        // certification path to a root-CA already trusted by the application
        // doing the verification. The record may just point to an intermediate
        // CA, in which case the certificate for this service must come via
        // this CA, but the entire chain to a trusted root-CA must still be
        // valid.
        certificate_authority_constraint = 0,

        // The certificate used must match the TLSA record exactly, and it must
        // also pass PKIX certification path validation to a trusted root-CA.
        service_certificate_constraint = 1,

        // The certificate used has a valid certification path pointing back to
        // the certificate mention in this record, but there is no need for it
        // to pass the PKIX certification path validation to a trusted root-CA.
        trust_anchor_assertion = 2,

        // The services uses a self-signed certificate. It is not signed by
        // anyone else, and is exactly this record.
        domain_issued_certificate = 3
    };

    /**
     *  When connecting to the service and a certificate is received, the
     *  selector field specifies which parts of it should be checked.
     */
    enum class Selector : uint8_t
    {
        // Select the entire certificate for matching.
        entire_certificate = 0,

        // Select just the public key for certificate matching. Matching the
        // public key is often sufficient, as this is likely to be unique.
        only_public_key = 1
    };

    /**
     *  Matching type
     */
    enum class MatchingType : uint8_t
    {
        // The entire information selected is present in the certificate
        // association data.
        info_is_in_certificate_association_data = 0,

        // Do a SHA-256 hash of the selected data.
        do_SHA256 = 1,

        // Do a SHA-512 hash of the selected data.
        do_SHA512 = 2
    };

private:
    /**
     *  The certificate usage
     *  @var CertificateUsage
     */
    CertificateUsage _usage;

    /**
     *  The selector
     *  @var Selector
     */
    Selector _selector;

    /**
     *  The matching type
     *  @var MatchingType
     */
    MatchingType _matching;

    /**
     *  The association data
     *  @var unsigned char *
     */
    const unsigned char *_association;

public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    TLSA(const Response &response, const Record &record) :
        Extractor(record, TYPE_TLSA, 3),
        _usage(static_cast<CertificateUsage>(record.data()[0])),
        _selector(static_cast<Selector>(record.data()[1])),
        _matching(static_cast<MatchingType>(record.data()[2])),
        _association(record.data() + 3) {}

    /**
     *  Destructor
     */
    virtual ~TLSA() = default;

    /**
     *  How to verify the certificate?
     *  @return the certificate usage
     */
    CertificateUsage certificateUsage() const { return _usage; }

    /**
     *  When a certificate is received, which parts of it should be checked?
     *  @return the selector
     */
    Selector selector() const { return _selector; }

    /**
     *  How do we declare it a match?
     *  @return the matching type
     */
    MatchingType matchingType() const { return _matching; }

    /**
     *  The actual data to be matched given the settings of the other fields.
     *  @return a text string of hexadecimal data
     */
    const unsigned char *certificateAssociationData() const { return _association; }

    /**
     *  The length of the certificate assocation data.
     *  @return length
     */
    size_t certificateAssociationDataSize() const { return _record.size() - 3; }
};

/**
 *  End of namespace
 */
}
