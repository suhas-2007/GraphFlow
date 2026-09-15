#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <concepts>
#include <limits>
#include <utility>
#include "graphflow/core/types.hpp"

namespace graphflow::core {

/**
 * @brief High-Performance 4-ary (d-ary) Indexed Min-Heap with Hole-Bubbling.
 *
 * Optimizations:
 * 1. 4-ary branching cuts tree depth by 50% vs binary heaps, fetching 4 sibling entries
 *    into a single contiguous L1 cache line.
 * 2. 32-bit uint32_t pos_in_heap_ cuts memory and cache pressure by 50%.
 * 3. Hole-bubbling in sift_up and sift_down eliminates unnecessary entry swaps.
 * 4. In-place decrease_key in O(log_4 N), eliminating redundant duplicate pushes.
 */
template <typename Key = NodeId, typename Priority = EdgeWeight, size_t D = 4>
requires (D >= 2)
class IndexedDaryHeap {
public:
    static constexpr uint32_t kInvalidPos = std::numeric_limits<uint32_t>::max();

    struct Entry {
        Key key;
        Priority priority;
    };

    explicit IndexedDaryHeap(size_t max_keys = 0) {
        if (max_keys > 0) {
            reserve(max_keys);
        }
    }

    void reserve(size_t max_keys) {
        heap_.reserve(max_keys);
        pos_in_heap_.assign(max_keys, kInvalidPos);
    }

    void clear() noexcept {
        for (const auto& entry : heap_) {
            if (static_cast<size_t>(entry.key) < pos_in_heap_.size()) {
                pos_in_heap_[static_cast<size_t>(entry.key)] = kInvalidPos;
            }
        }
        heap_.clear();
    }

    [[nodiscard]] inline bool empty() const noexcept {
        return heap_.empty();
    }

    [[nodiscard]] inline size_t size() const noexcept {
        return heap_.size();
    }

    [[nodiscard]] inline bool contains(Key key) const noexcept {
        size_t idx = static_cast<size_t>(key);
        return idx < pos_in_heap_.size() && pos_in_heap_[idx] != kInvalidPos;
    }

    [[nodiscard]] inline Priority get_priority(Key key) const noexcept {
        size_t idx = static_cast<size_t>(key);
        return heap_[pos_in_heap_[idx]].priority;
    }

    [[nodiscard]] inline const Entry& top() const noexcept {
        return heap_.front();
    }

    inline void push_or_decrease_key(Key key, Priority priority) {
        size_t idx = static_cast<size_t>(key);
        if (idx >= pos_in_heap_.size()) {
            pos_in_heap_.resize(idx + 1, kInvalidPos);
        }

        uint32_t pos = pos_in_heap_[idx];
        if (pos == kInvalidPos) {
            // New insertion at bottom
            pos = static_cast<uint32_t>(heap_.size());
            heap_.push_back({key, priority});
            pos_in_heap_[idx] = pos;
            sift_up(pos);
        } else if (priority < heap_[pos].priority) {
            heap_[pos].priority = priority;
            sift_up(pos);
        }
    }

    inline void push(Key key, Priority priority) {
        push_or_decrease_key(key, priority);
    }

    Entry pop() {
        Entry min_entry = heap_.front();
        pos_in_heap_[static_cast<size_t>(min_entry.key)] = kInvalidPos;

        if (heap_.size() == 1) {
            heap_.pop_back();
            return min_entry;
        }

        Entry last = heap_.back();
        heap_.pop_back();
        sift_down_hole(0, last);
        return min_entry;
    }

private:
    [[nodiscard]] static constexpr uint32_t parent(uint32_t i) noexcept {
        return (i - 1) / D;
    }

    [[nodiscard]] static constexpr uint32_t first_child(uint32_t i) noexcept {
        return static_cast<uint32_t>(D * i + 1);
    }

    void sift_up(uint32_t i) {
        Entry target = heap_[i];
        while (i > 0) {
            uint32_t p = parent(i);
            if (target.priority < heap_[p].priority) {
                heap_[i] = heap_[p];
                pos_in_heap_[static_cast<size_t>(heap_[i].key)] = i;
                i = p;
            } else {
                break;
            }
        }
        heap_[i] = target;
        pos_in_heap_[static_cast<size_t>(target.key)] = i;
    }

    void sift_down_hole(uint32_t i, Entry target) {
        const uint32_t n = static_cast<uint32_t>(heap_.size());
        while (true) {
            uint32_t c_start = first_child(i);
            if (c_start >= n) break;

            uint32_t c_end = std::min(c_start + static_cast<uint32_t>(D), n);
            uint32_t best = c_start;

            for (uint32_t c = c_start + 1; c < c_end; ++c) {
                if (heap_[c].priority < heap_[best].priority) {
                    best = c;
                }
            }

            if (heap_[best].priority < target.priority) {
                heap_[i] = heap_[best];
                pos_in_heap_[static_cast<size_t>(heap_[i].key)] = i;
                i = best;
            } else {
                break;
            }
        }
        if (i < heap_.size()) {
            heap_[i] = target;
            pos_in_heap_[static_cast<size_t>(target.key)] = i;
        } else {
            heap_.push_back(target);
            pos_in_heap_[static_cast<size_t>(target.key)] = i;
        }
    }

    std::vector<Entry> heap_;
    std::vector<uint32_t> pos_in_heap_;
};

} // namespace graphflow::core
