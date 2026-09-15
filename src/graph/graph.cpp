#include "graphflow/graph/graph.hpp"
#include <algorithm>
#include <stdexcept>

namespace graphflow::graph {

DynamicGraph::DynamicGraph(size_t num_nodes, bool directed)
    : num_nodes_(num_nodes), directed_(directed), adj_(num_nodes), flow_adj_(num_nodes) {}

void DynamicGraph::resize(size_t num_nodes) {
    num_nodes_ = num_nodes;
    adj_.resize(num_nodes);
    flow_adj_.resize(num_nodes);
}

void DynamicGraph::clear() {
    for (auto& edges : adj_) edges.clear();
    for (auto& fedges : flow_adj_) fedges.clear();
    num_edges_ = 0;
}

void DynamicGraph::add_edge(core::NodeId u, core::NodeId v, core::EdgeWeight weight) {
    if (u >= num_nodes_ || v >= num_nodes_) {
        resize(std::max(u, v) + 1);
    }

    adj_[u].push_back({v, weight});
    num_edges_++;

    if (!directed_) {
        adj_[v].push_back({u, weight});
    }
}

bool DynamicGraph::remove_edge(core::NodeId u, core::NodeId v) {
    if (u >= num_nodes_) return false;

    auto& edges = adj_[u];
    auto it = std::find_if(edges.begin(), edges.end(), [v](const core::Edge& e) {
        return e.target == v;
    });

    if (it != edges.end()) {
        edges.erase(it);
        num_edges_--;
        if (!directed_ && v < num_nodes_) {
            auto& rev_edges = adj_[v];
            auto rev_it = std::find_if(rev_edges.begin(), rev_edges.end(), [u](const core::Edge& e) {
                return e.target == u;
            });
            if (rev_it != rev_edges.end()) {
                rev_edges.erase(rev_it);
            }
        }
        return true;
    }
    return false;
}

void DynamicGraph::update_edge_weight(core::NodeId u, core::NodeId v, core::EdgeWeight new_weight) {
    if (u >= num_nodes_) return;

    for (auto& e : adj_[u]) {
        if (e.target == v) {
            e.weight = new_weight;
            break;
        }
    }
    if (!directed_ && v < num_nodes_) {
        for (auto& e : adj_[v]) {
            if (e.target == u) {
                e.weight = new_weight;
                break;
            }
        }
    }
}

void DynamicGraph::add_flow_edge(core::NodeId u, core::NodeId v, core::FlowType capacity, core::CostType cost) {
    if (u >= num_nodes_ || v >= num_nodes_) {
        resize(std::max(u, v) + 1);
    }

    uint32_t a_idx = static_cast<uint32_t>(flow_adj_[u].size());
    uint32_t b_idx = static_cast<uint32_t>(flow_adj_[v].size());

    // Forward edge
    flow_adj_[u].push_back(core::FlowEdge{
        .to = v,
        .capacity = capacity,
        .flow = 0,
        .cost = cost,
        .rev = b_idx
    });

    // Backward residual edge (0 capacity, negative cost)
    flow_adj_[v].push_back(core::FlowEdge{
        .to = u,
        .capacity = 0,
        .flow = 0,
        .cost = -cost,
        .rev = a_idx
    });
}

bool DynamicGraph::has_edge(core::NodeId u, core::NodeId v) const noexcept {
    if (u >= num_nodes_) return false;
    for (const auto& e : adj_[u]) {
        if (e.target == v) return true;
    }
    return false;
}

std::optional<core::EdgeWeight> DynamicGraph::get_edge_weight(core::NodeId u, core::NodeId v) const noexcept {
    if (u >= num_nodes_) return std::nullopt;
    for (const auto& e : adj_[u]) {
        if (e.target == v) return e.weight;
    }
    return std::nullopt;
}

CSRGraph DynamicGraph::to_csr() const {
    CSRGraph csr;
    csr.num_nodes = num_nodes_;
    csr.num_edges = 0;
    csr.row_ptrs.resize(num_nodes_ + 1, 0);

    for (size_t i = 0; i < num_nodes_; ++i) {
        csr.row_ptrs[i + 1] = csr.row_ptrs[i] + adj_[i].size();
        csr.num_edges += adj_[i].size();
    }

    csr.col_indices.resize(csr.num_edges);
    csr.weights.resize(csr.num_edges);

    for (size_t i = 0; i < num_nodes_; ++i) {
        size_t offset = csr.row_ptrs[i];
        for (size_t j = 0; j < adj_[i].size(); ++j) {
            csr.col_indices[offset + j] = adj_[i][j].target;
            csr.weights[offset + j] = adj_[i][j].weight;
        }
    }

    return csr;
}

} // namespace graphflow::graph
