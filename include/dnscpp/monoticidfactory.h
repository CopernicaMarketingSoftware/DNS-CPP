/**
 *  monotonicidfactory.h
 *
 *  Provides unique monotonically increasing numbers for queries.
 *
 *  @author Raoul Wols <raoul.wols@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include "abstractidfactory.h"
#include <limits>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Class declaration
 */
class MonoticIdFactory final : public AbstractIdFactory
{
    /**
     *  The current ID
     *  @var uint16_t
     */
    uint16_t _current = 0;

public:
    /**
     *  Destroys the object.
     */
    ~MonoticIdFactory() noexcept override = default;

    /**
     *  @see AbstractIdFactory::generate
     */
    uint16_t generate() override
    {
        // if it'll overflow, set it to one
        if (_current == std::numeric_limits<uint16_t>::max()) _current = 1;

        // otherwise just increment
        else ++_current;

        // return the value
        return _current;
    }

    /**
     *  @see AbstractIdFactory::free
     */
    void free(uint16_t id) override {}

    /**
     *  @see AbstractIdFactory::maxCapacity
     */
    uint16_t maxCapacity() const noexcept override { return std::numeric_limits<uint16_t>::max() - 1; }
};

/**
 *  End namespace
 */
}
