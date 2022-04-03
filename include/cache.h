#pragma once

#include <algorithm>
#include <cstddef>
#include <list>
#include <new>
#include <ostream>

template <class Key, class KeyProvider, class Allocator>
class Cache
{
    static_assert(std::is_constructible_v<KeyProvider, const Key &>, "KeyProvider has to be constructable from Key");

public:
    template <class... AllocArgs>
    Cache(const std::size_t cache_size, AllocArgs &&... alloc_args)
        : m_max_size(cache_size)
        , m_alloc(std::forward<AllocArgs>(alloc_args)...)
    {
    }

    std::size_t size() const
    {
        return m_data.size();
    }

    bool empty() const
    {
        return m_data.empty();
    }

    template <class T>
    T & get(const Key & key);

    std::ostream & print(std::ostream & strm) const;

    friend std::ostream & operator<<(std::ostream & strm, const Cache & cache)
    {
        return cache.print(strm);
    }

private:
    const std::size_t m_max_size;
    Allocator m_alloc;
    std::list<std::pair<KeyProvider *, bool>> m_data;
};

template <class Key, class KeyProvider, class Allocator>
template <class T>
inline T & Cache<Key, KeyProvider, Allocator>::get(const Key & key)
{
    auto it = std::find_if(m_data.begin(), m_data.end(), [&key](const std::pair<KeyProvider *, bool> value) {
        return *value.first == key;
    });

    if (it != m_data.end()) {
        it->second = true;
        return static_cast<T &>(*it->first);
    }
    else {
        while (m_max_size == size()) {
            if (m_data.back().second) {
                m_data.push_front({m_data.back().first, false});
            }
            else {
                m_alloc.destroy(m_data.back().first);
            }
            m_data.pop_back();
        }
        m_data.push_front({m_alloc.template create<T>(key), false});
        return static_cast<T &>(*m_data.front().first);
    }
}

template <class Key, class KeyProvider, class Allocator>
inline std::ostream & Cache<Key, KeyProvider, Allocator>::print(std::ostream & strm) const
{
    for (auto i : m_data) {
        strm << *i.first;
        if (i.first != m_data.back().first) {
            strm << " ";
        }
    }
    return strm;
}
