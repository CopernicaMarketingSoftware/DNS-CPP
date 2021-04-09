/**
 *  idfactory.h
 *
 *  Provides unique numbers for queries.
 *
 *  @author Raoul Wols <raoul.wols@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Deps
 */
#include <climits>
#include <cstdint>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Interface declaration
 */
class AbstractIdFactory
{
public:
    /**
     *  Return a new query ID
     *  @return uint16_t
     */
    virtual uint16_t generate() = 0;

    /**
     *  Free up a query ID that was previously generated with a call to `generate`
     *  @param id query ID
     */
    virtual void free(uint16_t id) = 0;

    /**
     *  Get the amount of query IDs maximally allowed in-flight
     *  @return uint16_t
     */
    virtual uint16_t maxCapacity() const noexcept = 0;

    /**
     *  Destroys the object.
     */
    virtual ~AbstractIdFactory() = default;
};

/**
 *  End of namespace DNS
 */
}
