/**
 *  IdGenerator.h
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
#include <climits>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Class declaration
 */
class IdGenerator
{
private:
    /**
     *  The pseudo-random number generator
     *  @var std::mt19937
     */
    std::mt19937 _engine;

    /**
     *  A uniform distribution for the range {1, 2, ..., 2^16 - 1}
     *  @var std::uniform_int_distribution<uint16_t>
     */
    std::uniform_int_distribution<uint16_t> _distribution;

public:
    /**
     *  Constructor takes an initial seed for the pseudo-random number generator
     *  @param seed
     */
    IdGenerator(std::random_device::result_type seed) :
        _engine(seed), _distribution(1, std::numeric_limits<uint16_t>::max()) {}
    
    /**
     *  Default constructor
     */
    IdGenerator() : IdGenerator(std::random_device{}()) {}

    /**
     *  Destroys the object.
     */
    virtual ~IdGenerator() noexcept = default;

    /**
     *  Return a new query ID
     *  @return uint16_t
     */
    uint16_t generate()
    {
        // get a random number
        return _distribution(_engine);
    }

    /**
     *  Get the number of query IDs maximally allowed in-flight
     *  @return uint16_t
     */
    static uint16_t capacity() noexcept
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
