/**
 *  randomizedidfactory.h
 *
 *  Provides unique random numbers for queries.
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
#include <random>
#include <set>
#include <cassert>
#include <climits>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Class declaration
 */
template <class PseudoRandomEngine = std::default_random_engine>
class RandomizedIdFactory final
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
    ~RandomizedIdFactory() noexcept = default;

    /**
     *  Return a new query ID
     *  @return uint16_t
     */
    uint16_t generate()
    {
        assert(_idsInUse.size() <= maxCapacity());

        // the value that we'll return
        uint16_t result;

        // loop forever
        while (true)
        {
            // get a random number
            result = _distribution(_engine);

            // if it's already in use, try again
            // because the maxCapacity is at most 2^15, this has at least a
            // 50% chance of not being in this set
            // however, it is vital that the `free` method is called at an
            // appropriate time for this assumption to work
            if (_idsInUse.count(result)) continue;

            // ok, remember that we found a free ID
            _idsInUse.insert(result);

            // return the ID
            return result;
        }
    }

    /**
     *  Free up a query ID that was previously generated with a call to `generate`
     *  @param id query ID
     */
    void free(uint16_t id) { _idsInUse.erase(id); }

    /**
     *  Get the amount of query IDs maximally allowed in-flight
     *  @return uint16_t
     */
    uint16_t maxCapacity() const noexcept
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
 *  End namespace
 */
}
