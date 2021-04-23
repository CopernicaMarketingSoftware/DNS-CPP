/**
 *  intrusivequeue.h
 *
 *  Queue datastructure which also allows O(1)
 *  removal from an item in the middle of the queue.
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
#include "intrusivequeueitem.h"
#include <type_traits>

/**
 *  Begin namespace
 */
namespace DNS {

/**
 *  Class declaration
 *  @tparam  T  The type of the items. The actual item type will be shared_ptr<T>.
 */
template <class T> class IntrusiveQueue final
{
public:
    static_assert(std::is_base_of<IntrusiveQueueItem<T>, T>::value, "must derive from IntrusiveQueueItem");

    /**
     *  Push a new item onto the back of the queue.
     *  @param  item Item to be inserted. The position of the item is remembered inside of the item itself.
     */
    void push(const std::shared_ptr<T> &item)
    {
        // insert it at the back
        _q.push_back(item);

        // update position
        item->position(std::prev(_q.end()));
    }

    /**
     *  Pop an item from the front of the queue.
     *  @return the popped item.
     */
    std::shared_ptr<T> pop()
    {
        // move it out from the front
        auto result = move(_q.front());

        // forget the position
        result->clearposition();

        // pop front item
        _q.pop_front();

        // done
        return result;
    }

    /**
     *  Pop an item somewhere in the middle of the queue.
     *  @param    item the item
     *  @return   true if the item was at the front of the queue, false otherwise
     */
    bool pop(IntrusiveQueueItem<T> &item)
    {
        // if it doesn't have a position then we can't remove it
        if (!item.hasposition()) return false;

        // get the remembered position from the item
        const auto pos = item.position();

        // check if we're removing an item at the front of the queue
        const bool atfront = pos == _q.begin();

        // remove it from the queue
        _q.erase(pos);

        // forget the position
        item.clearposition();

        // done
        return atfront;
    }

    /**
     *  Get a const reference to the item at the front of the queue.
     *  @return const-ref
     */
    typename std::list<std::shared_ptr<T>>::const_reference front() const noexcept { return _q.front(); }

    /**
     *  Size of the queue.
     *  @return size_t
     */
    size_t size() const noexcept { return _q.size(); }

    /**
     *  Is the queue empty?
     *  @return bool
     */
    bool empty() const noexcept { return _q.empty(); }

private:
    /**
     *  Because we need stable iterators, we chose an std::list
     *  as the underlying datastructure.
     *  @var std::list<std::shared_ptr<T>>
     */
    std::list<std::shared_ptr<T>> _q;
};

} // namespace DNS
