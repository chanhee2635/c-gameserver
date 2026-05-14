#pragma once

template<typename T>
class LockQueue
{
public:
    LockQueue() = default;
    LockQueue(const LockQueue&) = delete;
    LockQueue& operator=(const LockQueue&) = delete;

    void Push(T item)
    {
        WRITE_LOCK;
        _items.push(std::move(item));
    }

    bool TryPop(OUT T& item)
    {
        WRITE_LOCK;
        if (_items.empty())
            return false;

        item = std::move(_items.front());
        _items.pop();
        return true;
    }

    void PopAll(OUT Vector<T>& items)
    {
        WRITE_LOCK;
        if (_items.empty())
            return;

        while (!_items.empty())
        {
            items.push_back(std::move(_items.front()));
            _items.pop();
        }
    }

    void Clear()
    {
        WRITE_LOCK;
        Queue<T> empty;
        std::swap(_items, empty);
    }

    bool Empty()
    {
        READ_LOCK;
        return _items.empty();
    }

private:
    USE_LOCK;
    Queue<T> _items;
};
