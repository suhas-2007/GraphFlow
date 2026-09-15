#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <algorithm>
#include "graphflow/core/types.hpp"
#include "graphflow/core/bitset.hpp"

namespace graphflow::graph {

/**
 * @brief Cache-Optimized Bitset-Compressed Adjacency List.
 * Features:
 * 1. Cache-dense 8-byte CompactEdge (4-byte NodeId + 4-byte float weight).
 * 2. 64-bit hardware bitmask filter per node for 1-cycle O(1) edge rejection.
 * 3. Fast contiguous neighbor traversal with zero pointer indirection.
 */
class BitsetAdjacencyList {
public:
    struct CompactEdge {
        core::NodeId target;
        float weight;
    };

    explicit BitsetAdjacencyList(size_t num_nodes = 0);

    void resize(size_t num_nodes);
    void add_edge(core::NodeId u, core::NodeId v, float weight = 1.0f);

    [[nodiscard]] inline bool has_edge(core::NodeId u, core::NodeId v) const noexcept {
        if (u >= num_nodes_) return false;
        // 1-cycle bitmask filter check
        if ((bitmask_filters_[u] & (uint64_t{1} << (v & 63))) == 0) {
            return false;
        }
        for (const auto& e : edges_[u]) {
            if (e.target == v) return true;
        }
        return false;
    }

    [[nodiscard]] float get_weight(core::NodeId u, core::NodeId v, float default_val = 1.0f) const noexcept;

    [[nodiscard]] inline size_t num_nodes() const noexcept { return num_nodes_; }
    [[nodiscard]] inline size_t num_edges() const noexcept { return num_edges_; }

    [[nodiscard]] inline std::span<const CompactEdge> neighbors(core::NodeId u) const noexcept {
        if (u >= num_nodes_) return {};
        return edges_[u];
    }

    [[nodiscard]] inline size_t degree(core::NodeId u) const noexcept {
        if (u >= num_nodes_) return 0;
        return edges_[u].size();
    }

    [[nodiscard]] inline uint64_t bitmask_filter(core::NodeId u) const noexcept {
        if (u >= num_nodes_) return 0;
        return bitmask_filters_[u];
    }

    /**
     * @brief Fast common neighbors approximation/count using hardware bitmask intersection.
     */
    [[nodiscard]] size_t common_neighbors(core::NodeId u, core::NodeId v) const;

private:
    size_t num_nodes_{0};
    size_t num_edges_{0};
    std::vector<std::vector<CompactEdge>> edges_;
    std::vector<uint64_t> bitmask_filters_;
};

} // namespace graphflow::graph
