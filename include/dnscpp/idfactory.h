#pragma once

#include <climits>
#include <cstdint>
#include <random>
#include <set>

namespace DNS {

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

class MonoticIdFactory final : public AbstractIdFactory
{
    uint16_t _current = 0;
public:
    ~MonoticIdFactory() noexcept override = default;

    uint16_t generate() override
    {
        if (_current == std::numeric_limits<uint16_t>::max()) _current = 1;
        else ++_current;
        return _current;
    }
    void free(uint16_t id) override {}
    uint16_t maxCapacity() const noexcept override { return std::numeric_limits<uint16_t>::max() - 1; }
};

template <class PseudoRandomEngine = std::default_random_engine>
class RandomizedIdFactory final : public AbstractIdFactory
{
    PseudoRandomEngine _engine;
    std::uniform_int_distribution<uint16_t> _distribution;
    std::set<uint16_t> _idsInUse;

public:
    RandomizedIdFactory(std::random_device::result_type seed) :
        _engine(seed),
        _distribution(1, std::numeric_limits<uint16_t>::max())
    {
    }

    ~RandomizedIdFactory() noexcept override = default;

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

    void free(uint16_t id) override { _idsInUse.erase(id); }

    uint16_t maxCapacity() const noexcept override
    {
        // The maximum capacity is 2^15. This is so that the probability of
        // selecting a random free query ID is at least 50%. Although it's
        // likely that you'll need a massive receive buffer for this.
        constexpr const size_t bit15 = sizeof(uint16_t) * CHAR_BIT - 1;
        return 1u << bit15;
    }
};

}
