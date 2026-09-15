#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <optional>
#include "graphflow/core/types.hpp"

namespace graphflow::graph {

/**
 * @brief Compact Compressed Sparse Row (CSR) Representation.
 * Optimal cache locality for read-heavy graph traversals.
 */
struct CSRGraph {
    std::vector<uint64_t> row_ptrs;
    std::vector<core::NodeId> col_indices;
    std::vector<core::EdgeWeight> weights;
    size_t num_nodes{0};
    size_t num_edges{0};

    [[nodiscard]] inline size_t degree(core::NodeId u) const noexcept {
        return row_ptrs[u + 1] - row_ptrs[u];
    }

    [[nodiscard]] inline std::span<const core::NodeId> neighbors(core::NodeId u) const noexcept {
        return {col_indices.data() + row_ptrs[u], col_indices.data() + row_ptrs[u + 1]};
    }

    [[nodiscard]] inline std::span<const core::EdgeWeight> edge_weights(core::NodeId u) const noexcept {
        return {weights.data() + row_ptrs[u], weights.data() + row_ptrs[u + 1]};
    }
};

/**
 * @brief Dynamic Graph representation supporting runtime mutations,
 * edge additions/removals, flow residual network modeling, and CSR conversion.
 */
class DynamicGraph {
public:
    explicit DynamicGraph(size_t num_nodes = 0, bool directed = true);

    void resize(size_t num_nodes);
    void clear();

    void add_edge(core::NodeId u, core::NodeId v, core::EdgeWeight weight = 1.0);
    bool remove_edge(core::NodeId u, core::NodeId v);
    void update_edge_weight(core::NodeId u, core::NodeId v, core::EdgeWeight new_weight);

    // Flow network support
    void add_flow_edge(core::NodeId u, core::NodeId v, core::FlowType capacity, core::CostType cost = 0);

    [[nodiscard]] bool has_edge(core::NodeId u, core::NodeId v) const noexcept;
    [[nodiscard]] std::optional<core::EdgeWeight> get_edge_weight(core::NodeId u, core::NodeId v) const noexcept;

    [[nodiscard]] inline size_t num_nodes() const noexcept { return num_nodes_; }
    [[nodiscard]] inline size_t num_edges() const noexcept { return num_edges_; }
    [[nodiscard]] inline bool is_directed() const noexcept { return directed_; }

    [[nodiscard]] const std::vector<core::Edge>& out_edges(core::NodeId u) const {
        return adj_[u];
    }

    [[nodiscard]] std::vector<core::Edge>& out_edges(core::NodeId u) {
        return adj_[u];
    }

    [[nodiscard]] const std::vector<core::FlowEdge>& flow_edges(core::NodeId u) const {
        return flow_adj_[u];
    }

    [[nodiscard]] std::vector<core::FlowEdge>& flow_edges(core::NodeId u) {
        return flow_adj_[u];
    }

    [[nodiscard]] CSRGraph to_csr() const;

private:
    size_t num_nodes_{0};
    size_t num_edges_{0};
    bool directed_{true};
    std::vector<std::vector<core::Edge>> adj_;
    std::vector<std::vector<core::FlowEdge>> flow_adj_;
};

} // namespace graphflow::graph
