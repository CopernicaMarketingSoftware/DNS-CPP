#pragma once

#include <list>
#include <memory>

namespace DNS {

template <class T>
class IntrusiveQueueItem
{
private:
    typename std::list<std::shared_ptr<T>>::const_iterator _position;
    bool _hasposition = false;

protected:
    IntrusiveQueueItem() = default;
    virtual ~IntrusiveQueueItem() noexcept = default;
    IntrusiveQueueItem(const IntrusiveQueueItem&) = delete;
    IntrusiveQueueItem& operator=(const IntrusiveQueueItem&) = delete;

public:
    bool hasposition() const noexcept { return _hasposition; }
    typename std::list<std::shared_ptr<T>>::const_iterator position() const noexcept { return _position; }
    void position(const typename std::list<std::shared_ptr<T>>::const_iterator &value)
    {
        _position = value;
        _hasposition = true;
    }
    void clearposition()
    {
        _hasposition = false;
    }
};

}
