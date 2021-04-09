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
#include <random>
#include <set>

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
 *  Class declaration
 */
template <class PseudoRandomEngine = std::default_random_engine>
class RandomizedIdFactory final : public AbstractIdFactory
{
    /**
     *  The pseudo-random number generator
     *  @var PseudoRandomEngine
     */
    PseudoRandomEngine _engine;

    /**
     *  A uniform distribution for the range {1, 2, ..., 2^16 - 1}
     *  @var std::uniform_int_distribution<uint16_t>
     */
    std::uniform_int_distribution<uint16_t> _distribution;

    /**
     *  The query IDs currently in-flight
     *  @var std::set<uint16_t>
     */
    std::set<uint16_t> _idsInUse;

public:
    /**
     *  Constructor takes an initial seed for the pseudo-random number generator
     *  @param seed
     */
    RandomizedIdFactory(std::random_device::result_type seed) :
        _engine(seed),
        _distribution(1, std::numeric_limits<uint16_t>::max())
    {
    }

    /**
     *  Destroys the object.
     */
    ~RandomizedIdFactory() noexcept override = default;

    /**
     *  @see AbstractIdFactory::generate
     */
    uint16_t generate() override
    {
        uint16_t result;
        while (true)
        {
            result = _distribution(_engine);
            if (_idsInUse.count(result)) continue;
            _idsInUse.insert(result);
            return result;
        }
    }

    /**
     *  @see AbstractIdFactory::free
     */
    void free(uint16_t id) override { _idsInUse.erase(id); }

    /**
     *  @see AbstractIdFactory::maxCapacity
     */
    uint16_t maxCapacity() const noexcept override
    {
        // The maximum capacity is 2^15. This is so that the probability of
        // selecting a random free query ID is at least 50%. Although it's
        // likely that you'll need a massive receive buffer for this.
        constexpr const size_t bit15 = sizeof(uint16_t) * CHAR_BIT - 1;

        // return the value
        return 1u << bit15;
    }
};

/**
 *  End of namespace DNS
 */
}
