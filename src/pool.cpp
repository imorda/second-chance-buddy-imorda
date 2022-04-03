#include "pool.h"

#include <cassert>
#include <functional>

void * PoolAllocator::allocate(const std::size_t n)
{
    const unsigned target_p = std::max(upper_bin_power(n), m_min_p);
    auto candidate = get_suitable_block(target_p, m_block_map_root); // Guarantees that candidate.node is Leaf

    Leaf * canditate_leaf = static_cast<Leaf *>(candidate.node);
    unsigned power = candidate.power;

    if (is_leaf(candidate.node)) {
        while (power > target_p) {
            canditate_leaf = split_block(canditate_leaf);
            power--;
        }
        if (canditate_leaf != nullptr) {
            canditate_leaf->used = true;
        }
        return &m_storage[candidate.offset];
    }
    throw std::bad_alloc{};
}

void PoolAllocator::deallocate(const void * ptr)
{
    auto b_ptr = static_cast<const std::byte *>(ptr);

    Block block = find_by_offset(b_ptr - &m_storage[0], m_block_map_root); // Guarantees that block.node is Leaf
    Leaf * leaf = static_cast<Leaf *>(block.node);

    if (is_leaf(block.node)) {
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

    Leaf * leaf_cast = static_cast<Leaf *>(current.node);
    if (is_leaf(current.node)) {
        if (leaf_cast->used) {
            return {nullptr, static_cast<unsigned>(-1), size_t_max_val};
        }
        return current;
    }

    Block left = get_suitable_block(k, {current.node->left, current.power - 1, current.offset});

    if (left.power == k) {
        return left;
    }

    Block right = get_suitable_block(k, {current.node->right, current.power - 1, current.offset + (1UL << (current.power - 1))});

    if (right.power == k) {
        return right;
    }

    if (left.node != nullptr) {
        return left;
    }

    return right;
}

Leaf * PoolAllocator::split_block(Leaf * leaf)
{
    if (leaf == nullptr || leaf->used) {
        return nullptr;
    }

    Node * new_parent = new Node{};

    Leaf * ans = new Leaf(false, new_parent);
    new_parent->left = ans;
    new_parent->right = new Leaf(false, new_parent);

    if (leaf->parent == nullptr) {
        m_block_map_root.node = new_parent;
    }
    else {
        new_parent->parent = leaf->parent;
        if (leaf->parent->left == leaf) {
            leaf->parent->left = new_parent;
        }
        else {
            leaf->parent->right = new_parent;
        }
    }
    delete leaf;

    return ans;
}

Leaf * PoolAllocator::try_merge_block(Leaf * node)
{
    if (node == nullptr || node->parent == nullptr) {
        return node;
    }

    Leaf * parent_left = static_cast<Leaf *>(node->parent->left);
    Leaf * parent_right = static_cast<Leaf *>(node->parent->right);

    if (!is_leaf(node->parent->left) || !is_leaf(node->parent->right) || parent_left->used || parent_right->used) {
        return node;
    }

    Node * old_parent = node->parent;
    Leaf * merged = new Leaf(false, old_parent->parent);

    if (old_parent->parent != nullptr) {
        if (old_parent->parent->left == old_parent) {
            old_parent->parent->left = merged;
        }
        else {
            old_parent->parent->right = merged;
        }
    }
    else {
        m_block_map_root.node = merged;
    }

    delete old_parent;

    return merged;
}

/// Searches for a Block with a defined offset where node points to a Leaf
Block PoolAllocator::find_by_offset(std::size_t target_offset, Block current)
{
    assert(current.node != nullptr);
    if (is_leaf(current.node)) {
        assert(current.offset == target_offset);
        return current;
    }
    if (target_offset >= current.offset + (1UL << (current.power - 1))) {
        return find_by_offset(target_offset, {current.node->right, current.power - 1, current.offset + (1UL << (current.power - 1))});
    }
    return find_by_offset(target_offset, {current.node->left, current.power - 1, current.offset});
}
