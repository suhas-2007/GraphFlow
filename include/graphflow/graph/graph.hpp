#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <optional>
#include "graphflow/core/types.hpp"

namespace graphflow::graph {

using namespace core;

// Compressed Sparse Row format for static or read-heavy workloads
struct CSRGraph {
    std::vector<uint64_t> row_ptrs;
    std::vector<NodeId> col_indices;
    std::vector<EdgeWeight> weights;
    size_t num_nodes{0};
    size_t num_edges{0};

    [[nodiscard]] inline size_t degree(NodeId u) const noexcept {
        return row_ptrs[u + 1] - row_ptrs[u];
    }

    [[nodiscard]] inline std::span<const NodeId> neighbors(NodeId u) const noexcept {
        return {col_indices.data() + row_ptrs[u], col_indices.data() + row_ptrs[u + 1]};
    }

    [[nodiscard]] inline std::span<const EdgeWeight> edge_weights(NodeId u) const noexcept {
        return {weights.data() + row_ptrs[u], weights.data() + row_ptrs[u + 1]};
    }
};

// Dynamic adjacency list supporting edge mutations and residual flow networks
class DynamicGraph {
public:
    explicit DynamicGraph(size_t num_nodes = 0, bool directed = true);

    void resize(size_t num_nodes);
    void clear();

    void add_edge(NodeId u, NodeId v, EdgeWeight weight = 1.0);
    bool remove_edge(NodeId u, NodeId v);
    void update_edge_weight(NodeId u, NodeId v, EdgeWeight new_weight);

    void add_flow_edge(NodeId u, NodeId v, FlowType capacity, CostType cost = 0);

    [[nodiscard]] bool has_edge(NodeId u, NodeId v) const noexcept;
    [[nodiscard]] std::optional<EdgeWeight> get_edge_weight(NodeId u, NodeId v) const noexcept;

    [[nodiscard]] inline size_t num_nodes() const noexcept { return num_nodes_; }
    [[nodiscard]] inline size_t num_edges() const noexcept { return num_edges_; }
    [[nodiscard]] inline bool is_directed() const noexcept { return directed_; }

    [[nodiscard]] const std::vector<Edge>& out_edges(NodeId u) const {
        return adj_[u];
    }

    [[nodiscard]] std::vector<Edge>& out_edges(NodeId u) {
        return adj_[u];
    }

    [[nodiscard]] const std::vector<FlowEdge>& flow_edges(NodeId u) const {
        return flow_adj_[u];
    }

    [[nodiscard]] std::vector<FlowEdge>& flow_edges(NodeId u) {
        return flow_adj_[u];
    }

    [[nodiscard]] CSRGraph to_csr() const;

private:
    size_t num_nodes_{0};
    size_t num_edges_{0};
    bool directed_{true};
    std::vector<std::vector<Edge>> adj_;
    std::vector<std::vector<FlowEdge>> flow_adj_;
};

} // namespace graphflow::graph
