#pragma once

#include <cstddef>
#include <new>
#include <vector>

struct Node
{
    Node * parent = nullptr;
    Node * left = nullptr;
    Node * right = nullptr;

    ~Node();

    Node() = default;

    Node(Node * _parent)
        : parent(_parent)
    {
    }
};

struct Leaf : Node
{
    bool used;

    Leaf(bool used)
        : used(used)
    {
    }

    Leaf(bool used, Node * _parent)
        : Node(_parent)
        , used(used)
    {
    }
};

inline Node::~Node()
{
    if (left != nullptr) {
        if (left->left == nullptr && left->right == nullptr) {
            delete static_cast<Leaf *>(left);
        }
        else {
            delete left;
        }
    }
    if (right != nullptr) {
        if (right->left == nullptr && right->right == nullptr) {
            delete static_cast<Leaf *>(right);
        }
        else {
            delete right;
        }
    }
}

struct Block
{
    Node * node;
    unsigned power;
    std::size_t offset;
};

class PoolAllocator
{
public:
    PoolAllocator(const unsigned min_p, const unsigned max_p)
        : m_min_p(min_p)
        , m_storage(1UL << max_p)
        , m_block_map_root{new Leaf(false), max_p, 0}
    {
    }

    ~PoolAllocator()
    {
        delete m_block_map_root.node;
    }

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

    static bool is_leaf(Node * node)
    {
        return node != nullptr && node->left == nullptr && node->right == nullptr;
    }
};
