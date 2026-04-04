/**
 * @file PersistentMap.h
 * @brief Lock-free persistent hash map using a Hash Array Mapped Trie (HAMT).
 *
 * Structural sharing: insert/erase copy only the nodes along the mutation
 * path (~5-6 nodes, ~320 bytes), not the entire map. Old and new versions
 * share all unchanged subtrees via shared_ptr.
 *
 * Snapshots are free — just copy the root shared_ptr.
 * All versions are immutable once published, so concurrent readers are safe.
 */
#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

template <typename K, typename V, typename Hash = std::hash<K>>
class PersistentMap {
  public:
    PersistentMap() = default;

    /// Insert or update a key. Returns a new map that shares structure.
    PersistentMap set(const K& key, V value) const {
        size_t h = hash(key);
        bool inserted = false;
        auto new_root = set_impl(root_, 0, h, key, std::move(value), inserted);
        return {std::move(new_root), size_ + (inserted ? 1 : 0)};
    }

    /// Remove a key. Returns a new map. No-op if key not found.
    PersistentMap erase(const K& key) const {
        if (!root_)
            return *this;
        size_t h = hash(key);
        bool erased = false;
        auto new_root = erase_impl(root_, 0, h, key, erased);
        return {std::move(new_root), size_ - (erased ? 1 : 0)};
    }

    /// Find a value by key. Returns nullptr if not found.
    const V* find(const K& key) const {
        return find_impl(root_.get(), 0, hash(key), key);
    }

    size_t size() const {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }

    // -- Iteration -----------------------------------------------------------

    /// Key-value pair reference for iteration.
    struct Entry {
        const K& key;
        const V& value;
    };

  private:
    // Forward-declare Node so const_iterator can reference it.
    static constexpr int BITS = 5;
    static constexpr int WIDTH = 1 << BITS;
    static constexpr int MASK = WIDTH - 1;
    static constexpr int MAX_DEPTH = (sizeof(size_t) * 8 + BITS - 1) / BITS;

    struct Node {
        uint32_t bitmap = 0;
        struct NodeEntry {
            std::shared_ptr<const Node> child;
            K key;
            V value;
        };
        std::vector<NodeEntry> entries;

        bool has(int slot) const { return (bitmap >> slot) & 1; }
        int index(int slot) const { return std::popcount(bitmap & ((1u << slot) - 1)); }
    };
    using NodePtr = std::shared_ptr<const Node>;

  public:
    class const_iterator {
      public:
        using value_type = Entry;

        Entry operator*() const {
            auto& e = stack_.back().node->entries[stack_.back().idx];
            return {e.key, e.value};
        }

        const_iterator& operator++() {
            advance();
            return *this;
        }

        bool operator==(const const_iterator& o) const {
            return stack_.size() == o.stack_.size() &&
                   (stack_.empty() || (stack_.back().node == o.stack_.back().node &&
                                      stack_.back().idx == o.stack_.back().idx));
        }
        bool operator!=(const const_iterator& o) const {
            return !(*this == o);
        }

      private:
        friend class PersistentMap;
        struct Frame {
            const Node* node;
            int idx;
        };
        std::vector<Frame> stack_;

        void descend_to_leaf() {
            while (!stack_.empty()) {
                auto& top = stack_.back();
                while (top.idx < static_cast<int>(top.node->entries.size())) {
                    auto& e = top.node->entries[top.idx];
                    if (e.child) {
                        // Push branch, continue descending
                        auto* child = e.child.get();
                        ++top.idx;
                        stack_.push_back({child, 0});
                        // top reference is invalidated, restart inner loop
                        break;
                    }
                    else {
                        return; // at a leaf
                    }
                }
                // If we exhausted this node's entries, pop up
                if (stack_.back().idx >= static_cast<int>(stack_.back().node->entries.size()))
                    stack_.pop_back();
            }
        }

        void advance() {
            if (stack_.empty())
                return;
            ++stack_.back().idx; // move past current leaf
            descend_to_leaf();
        }
    };

    const_iterator begin() const {
        const_iterator it;
        if (root_) {
            it.stack_.push_back({root_.get(), 0});
            it.descend_to_leaf();
        }
        return it;
    }

    const_iterator end() const {
        return {};
    }

    /// Call a function for each key-value pair.
    template <typename F>
    void for_each(F&& fn) const {
        if (root_)
            for_each_impl(root_.get(), fn);
    }

    /// Cumulative bytes allocated for HAMT node copies across all instances.
    /// Shared across all PersistentMap<K,V,Hash> instantiations of the same types.
    static uint64_t copy_bytes_total() {
        return copy_bytes_.load(std::memory_order_relaxed);
    }

  private:
    NodePtr root_;
    size_t size_ = 0;
    static inline std::atomic<uint64_t> copy_bytes_{0};

    PersistentMap(NodePtr root, size_t size) : root_(std::move(root)), size_(size) {
    }

    /// Track a node copy. Called each time make_shared<Node> copies a node.
    static void track_node_copy(const Node& n) {
        // Node overhead: bitmap + vector header + entries array
        size_t bytes = sizeof(Node) + n.entries.size() * sizeof(typename Node::NodeEntry);
        copy_bytes_.fetch_add(bytes, std::memory_order_relaxed);
    }

    static size_t hash(const K& key) {
        return Hash{}(key);
    }

    static int slot_at(size_t h, int depth) {
        return (h >> (depth * BITS)) & MASK;
    }

    // -- Insert --------------------------------------------------------------

    static NodePtr set_impl(const NodePtr& node, int depth, size_t h, const K& key, V value, bool& inserted) {
        int slot = slot_at(h, depth);

        if (!node) {
            // Create a new single-leaf node
            auto n = std::make_shared<Node>();
            n->bitmap = 1u << slot;
            n->entries.push_back({nullptr, key, std::move(value)});
            inserted = true;
            return n;
        }

        // Copy this node (only the node itself, children are shared)
        auto n = std::make_shared<Node>(*node);
        track_node_copy(*n);
        int idx = n->index(slot);

        if (!n->has(slot)) {
            // Empty slot — insert leaf
            n->bitmap |= (1u << slot);
            n->entries.insert(n->entries.begin() + idx, {nullptr, key, std::move(value)});
            inserted = true;
            return n;
        }

        auto& entry = n->entries[idx];

        if (entry.child) {
            // Branch — recurse
            n->entries[idx].child = set_impl(entry.child, depth + 1, h, key, std::move(value), inserted);
            return n;
        }

        // Leaf
        if (entry.key == key) {
            // Update existing
            n->entries[idx].value = std::move(value);
            inserted = false;
            return n;
        }

        // Collision — push both leaves down one level
        if (depth >= MAX_DEPTH - 1) {
            // Hash exhausted — store as a second entry in a collision-style node
            // (extremely rare for well-distributed hashes)
            n->bitmap |= (1u << ((slot + 1) & MASK));
            int new_idx = n->index((slot + 1) & MASK);
            n->entries.insert(n->entries.begin() + new_idx, {nullptr, key, std::move(value)});
            inserted = true;
            return n;
        }

        size_t existing_hash = hash(entry.key);
        NodePtr branch;
        bool dummy = false;
        branch = set_impl(branch, depth + 1, existing_hash, entry.key, std::move(entry.value), dummy);
        branch = set_impl(branch, depth + 1, h, key, std::move(value), inserted);
        n->entries[idx] = {std::move(branch), K{}, V{}};
        return n;
    }

    // -- Erase ---------------------------------------------------------------

    static NodePtr erase_impl(const NodePtr& node, int depth, size_t h, const K& key, bool& erased) {
        if (!node)
            return nullptr;

        int slot = slot_at(h, depth);
        if (!node->has(slot))
            return node; // not found

        auto n = std::make_shared<Node>(*node);
        track_node_copy(*n);
        int idx = n->index(slot);
        auto& entry = n->entries[idx];

        if (entry.child) {
            // Branch — recurse
            auto new_child = erase_impl(entry.child, depth + 1, h, key, erased);
            if (!erased)
                return node; // nothing changed, return original

            n->entries[idx].child = std::move(new_child);

            // If child became empty, remove slot
            if (!n->entries[idx].child) {
                n->bitmap &= ~(1u << slot);
                n->entries.erase(n->entries.begin() + idx);
            }
            // If this node now has zero entries, return null
            if (n->entries.empty())
                return nullptr;
            return n;
        }

        // Leaf
        if (entry.key != key)
            return node; // different key, not found

        erased = true;
        n->bitmap &= ~(1u << slot);
        n->entries.erase(n->entries.begin() + idx);
        if (n->entries.empty())
            return nullptr;
        return n;
    }

    // -- Find ----------------------------------------------------------------

    static const V* find_impl(const Node* node, int depth, size_t h, const K& key) {
        if (!node)
            return nullptr;

        int slot = slot_at(h, depth);
        if (!node->has(slot))
            return nullptr;

        int idx = node->index(slot);
        auto& entry = node->entries[idx];

        if (entry.child)
            return find_impl(entry.child.get(), depth + 1, h, key);

        return entry.key == key ? &entry.value : nullptr;
    }

    // -- Iteration -----------------------------------------------------------

    template <typename F>
    static void for_each_impl(const Node* node, F& fn) {
        for (auto& e : node->entries) {
            if (e.child)
                for_each_impl(e.child.get(), fn);
            else
                fn(e.key, e.value);
        }
    }
};
