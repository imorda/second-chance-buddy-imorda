#pragma once

#include <cstddef>
#include <new>
#include <vector>

struct Node
{
    Node * parent = nullptr;
    virtual ~Node();

    Node();

    Node(Node *);
};

struct Internal : Node
{
    Node * left = nullptr;
    Node * right = nullptr;

    virtual ~Internal();

    Internal();
};

struct Leaf : Node
{
    bool used;

    virtual ~Leaf();

    Leaf(bool);
    Leaf(bool, Node *);
};

struct Block
{
    Node * node;
    unsigned power;
    std::size_t offset;
};

class PoolAllocator
{
public:
    PoolAllocator(unsigned min_p, unsigned max_p);

    ~PoolAllocator();

    void * allocate(std::size_t);

    void deallocate(const void *);

private:
    static constexpr std::size_t size_t_max_val = static_cast<std::size_t>(-1);

    static Block get_suitable_block(unsigned k, Block current);

    Leaf * split_block(Leaf * leaf);

    Leaf * try_merge_block(Leaf * node);

    static Block find_by_offset(std::size_t target_offset, Block current);

    const unsigned m_min_p;
    std::vector<std::byte> m_storage;
    Block m_block_map_root;

    static unsigned upper_bin_power(const std::size_t n)
    {
        unsigned i = 0;
        while ((1UL << i) < n) {
            ++i;
        }
        return i;
    }
};
