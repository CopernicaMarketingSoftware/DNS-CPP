#pragma once

#include "intrusivequeueitem.h"
#include <type_traits>

namespace DNS {

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
        _q.push_back(item);
        item->position(std::prev(_q.end()));
    }

    /**
     *  Pop an item from the front of the queue.
     *  @return the popped item.
     */
    std::shared_ptr<T> pop()
    {
        auto result = _q.front();
        result->clearposition();
        _q.pop_front();
        return result;
    }

    /**
     *  Pop an item somewhere in the middle of the queue.
     *  @param    item the item
     *  @return   true if the item was at the front of the queue, false otherwise
     */
    bool pop(IntrusiveQueueItem<T> &item)
    {
        if (!item.hasposition()) return false;
        const auto pos = item.position();
        const bool atfront = pos == _q.begin();
        _q.erase(pos);
        item.clearposition();
        return atfront;
    }

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
    std::list<std::shared_ptr<T>> _q;
};

} // namespace DNS
