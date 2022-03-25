#pragma once

#include <cstddef>
#include <new>

class PoolAllocator
{
public:
    PoolAllocator(const unsigned /*min_p*/, const unsigned /*max_p*/)
    {
    }

    void * allocate(const std::size_t /*n*/)
    {
        throw std::bad_alloc{};
    }

    void deallocate(const void * /*ptr*/)
    {
    }
};
