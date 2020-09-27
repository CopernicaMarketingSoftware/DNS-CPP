/**
 *  SOA.h
 *
 *  Helper class to parse the content in a SOA record
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
#include "extractor.h"
#include "decompressed.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class SOA : public Extractor
{
private:
    /**
     *  The name of the nameserver that is the original or primary source for the domain
     *  @var Decompressed
     */
    Decompressed _nameserver;

    /**
     *  The email address of the administrator
     *  @var Decompressed
     */
    Decompressed _email;
    

public:
    /**
     *  The constructor
     *  @param  response        the response from which the record was extracted
     *  @param  record          the record holding the CNAME
     *  @throws std::runtime_error
     */
    SOA(const Response &response, const Record &record) : 
        Extractor(record, ns_t_soa, 20),
        _nameserver(response, record.data()),
        _email     (response, record.data() + _nameserver.consumed()) {}
    
    /**
     *  Destructor
     */
    virtual ~SOA() = default;
    
    /**
     *  The name of the nameserver that is the original or primary source for the domain
     *  @return const char *
     */
    const char *nameserver() const { return _nameserver; }

    /**
     *  The email address of the administrator
     *  @return const char *
     */
    const char *email() const { return _email; }
    
    /**
     *  Serial number
     *  @return uint32_t
     */
    uint32_t serial() const { return ns_get32(_record.data() + _nameserver.consumed() + _email.consumed()); }
    
    /**
     *  Interval before zone should be refreshed
     *  @return uint32_t
     */
    uint32_t interval() const { return ns_get32(_record.data() + _nameserver.consumed() + _email.consumed() + 4); }
    
    /**
     *  Wait period between retries if a refresh fails
     *  @return uint32_t
     */
    uint32_t retry() const { return ns_get32(_record.data() + _nameserver.consumed() + _email.consumed() + 8); }
    
    /**
     *  The upper limit on the interval that can elapse before the zone is no longer authoritative.
     *  @return uint32_t
     */
    uint32_t expire() const { return ns_get32(_record.data() + _nameserver.consumed() + _email.consumed() + 12); }
    
    /**
     *  Minimum TTL for records in this zone
     *  @return uint32_t
     */
    uint32_t minimum() const { return ns_get32(_record.data() + _nameserver.consumed() + _email.consumed() + 16); }
};
    
/**
 *  End of namespace
 */
}

