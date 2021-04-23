/**
 *  intrusivequeueitem.h
 *
 *  Mixin class to allow removal from an IntrusiveQueue from anywhere
 *  in the queue.
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
#include <list>
#include <memory>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Class declaration
 *  @tparam  T  The derived type (your class)
 */
template <class T>
class IntrusiveQueueItem
{
private:

    /**
     *  The position in the queue. Because we need stable iterators,
     *  we chose a list as the underlying data structure for the queue.
     *  @var iterator
     */
    typename std::list<std::shared_ptr<T>>::const_iterator _position;

    /**
     *  Whether this item is inserted into a queue
     *  @var bool
     */
    bool _hasposition = false;

protected:
    /**
     *  Constructors are protected and copying is prohibited.
     */
    IntrusiveQueueItem() = default;
    virtual ~IntrusiveQueueItem() noexcept = default;
    IntrusiveQueueItem(const IntrusiveQueueItem&) = delete;
    IntrusiveQueueItem& operator=(const IntrusiveQueueItem&) = delete;

public:

    /**
     *  Whether this item is sitting in a queue.
     *  @return bool
     */
    bool hasposition() const noexcept { return _hasposition; }

    /**
     *  Get the iterator
     *  @return iterator
     */
    typename std::list<std::shared_ptr<T>>::const_iterator position() const noexcept { return _position; }

    /**
     *  Set the position in the queue.
     *  @param  value  iterator
     */
    void position(const typename std::list<std::shared_ptr<T>>::const_iterator &value)
    {
        _position = value;
        _hasposition = true;
    }

    /**
     *  Clear the position in the queue.
     */
    void clearposition()
    {
        _hasposition = false;
    }
};

/**
 *  End namespace DNS
 */
}
