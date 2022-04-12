#include "pool.h"

#include <cassert>
#include <functional>
#include <stdexcept>

Node::Node() = default;

Node::Node(Node * _parent)
    : parent(_parent)
{
}

Node::~Node() = default;

Leaf::Leaf(bool used)
    : used(used)
{
}

Leaf::Leaf(bool used, Node * _parent)
    : Node(_parent)
    , used(used)
{
}

Leaf::~Leaf() = default;

Internal::Internal() = default;

Internal::~Internal()
{
    delete left;
    delete right;
}

void * PoolAllocator::allocate(const std::size_t n)
{
    const unsigned target_p = std::max(upper_bin_power(n), m_min_p);
    auto candidate = get_suitable_block(target_p, m_block_map_root); // Guarantees that candidate.node is Leaf

    unsigned power = candidate.power;

    Leaf * candidate_leaf = dynamic_cast<Leaf *>(candidate.node);
    if (candidate_leaf != nullptr) {
        while (power > target_p) {
            candidate_leaf = split_block(candidate_leaf);
            power--;
        }
        candidate_leaf->used = true;
        return &m_storage[candidate.offset];
    }
    throw std::bad_alloc{};
}

void PoolAllocator::deallocate(const void * ptr)
{
    auto b_ptr = static_cast<const std::byte *>(ptr);

    Block block = find_by_offset(b_ptr - &m_storage[0], m_block_map_root); // Guarantees that block.node is Leaf
    Leaf * leaf = dynamic_cast<Leaf *>(block.node);

    if (leaf != nullptr) { // Needed to avoid warnings of nullptr dereference
        leaf->used = false;

        Leaf * new_leaf = leaf;

        do {
            leaf = new_leaf;
            new_leaf = try_merge_block(leaf);
        } while (new_leaf != leaf);
    }
}

/// Returns a best suitable for 'k' Block where node points to a Leaf
Block PoolAllocator::get_suitable_block(const unsigned k, Block current)
{
    if (current.node == nullptr || current.power < k) {
        return {nullptr, static_cast<unsigned>(-1), size_t_max_val};
    }

    Leaf * leaf_cast = dynamic_cast<Leaf *>(current.node);
    if (leaf_cast != nullptr) {
        if (leaf_cast->used) {
            return {nullptr, static_cast<unsigned>(-1), size_t_max_val};
        }
        return current;
    }

    Internal * internal_cast = dynamic_cast<Internal *>(current.node);

    if (internal_cast != nullptr) {
        Block left = get_suitable_block(k, {internal_cast->left, current.power - 1, current.offset});

        if (left.power == k) {
            return left;
        }

        Block right = get_suitable_block(k, {internal_cast->right, current.power - 1, current.offset + (1UL << (current.power - 1))});

        return left.power > right.power ? right : left;
    }
    throw std::logic_error("Got neither Leaf nor Node instance of Node");
}

Leaf * PoolAllocator::split_block(Leaf * leaf)
{
    if (leaf == nullptr || leaf->used) {
        return nullptr;
    }

    Internal * new_parent = new Internal{};

    Leaf * ans = new Leaf(false, new_parent);
    new_parent->left = ans;
    new_parent->right = new Leaf(false, new_parent);

    if (leaf->parent == nullptr) {
        m_block_map_root.node = new_parent;
    }
    else {
        new_parent->parent = leaf->parent;

        Internal * grandparent = dynamic_cast<Internal *>(leaf->parent);

        if (grandparent == nullptr) {
            throw std::logic_error("Got neither Leaf nor Node instance of Node");
        }

        if (grandparent->left == leaf) {
            grandparent->left = new_parent;
        }
        else {
            grandparent->right = new_parent;
        }
    }
    delete leaf;

    return ans;
}

Leaf * PoolAllocator::try_merge_block(Leaf * node)
{
    if (node == nullptr) {
        return node;
    }

    Internal * parent = dynamic_cast<Internal *>(node->parent);

    if (parent == nullptr) {
        return node;
    }

    Leaf * parent_left = dynamic_cast<Leaf *>(parent->left);
    Leaf * parent_right = dynamic_cast<Leaf *>(parent->right);
    if (parent_left == nullptr || parent_right == nullptr || parent_left->used || parent_right->used) {
        return node;
    }

    Leaf * merged = new Leaf(false, parent->parent);

    Internal * grandparent = dynamic_cast<Internal *>(parent->parent);

    if (grandparent != nullptr) {
        if (grandparent->left == parent) {
            grandparent->left = merged;
        }
        else {
            grandparent->right = merged;
        }
    }
    else {
        m_block_map_root.node = merged;
    }

    delete parent;

    return merged;
}

/// Searches for a Block with a defined offset where node points to a Leaf
Block PoolAllocator::find_by_offset(std::size_t target_offset, Block current)
{
    assert(current.node != nullptr);
    if (dynamic_cast<Leaf *>(current.node) != nullptr) {
        assert(current.offset == target_offset);
        return current;
    }

    Internal * node = dynamic_cast<Internal *>(current.node);
    if (node == nullptr) {
        throw std::logic_error("Got neither Leaf nor Node instance of Node");
    }

    if (target_offset >= current.offset + (1UL << (current.power - 1))) {
        return find_by_offset(target_offset, {node->right, current.power - 1, current.offset + (1UL << (current.power - 1))});
    }
    return find_by_offset(target_offset, {node->left, current.power - 1, current.offset});
}

PoolAllocator::PoolAllocator(const unsigned min_p, const unsigned max_p)
    : m_min_p(min_p)
    , m_storage(1UL << max_p)
    , m_block_map_root{new Leaf(false), max_p, 0}
{
}

PoolAllocator::~PoolAllocator()
{
    delete m_block_map_root.node;
}
