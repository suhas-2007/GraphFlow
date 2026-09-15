#include "graphflow/graph/bitset_adj_list.hpp"
#include <bit>
#include <algorithm>

using namespace std;

namespace graphflow::graph {

BitsetAdjacencyList::BitsetAdjacencyList(size_t num_nodes) {
    if (num_nodes > 0) {
        resize(num_nodes);
    }
}

void BitsetAdjacencyList::resize(size_t num_nodes) {
    num_nodes_ = num_nodes;
    edges_.resize(num_nodes);
    bitmask_filters_.resize(num_nodes, 0);
}

void BitsetAdjacencyList::add_edge(core::NodeId u, core::NodeId v, float weight) {
    if (u >= num_nodes_ || v >= num_nodes_) {
        resize(max(u, v) + 1);
    }

    edges_[u].push_back(CompactEdge{.target = v, .weight = weight});
    bitmask_filters_[u] |= (uint64_t{1} << (v & 63));
    num_edges_++;
}

float BitsetAdjacencyList::get_weight(core::NodeId u, core::NodeId v, float default_val) const noexcept {
    if (u >= num_nodes_) return default_val;
    if ((bitmask_filters_[u] & (uint64_t{1} << (v & 63))) == 0) {
        return default_val;
    }
    for (const auto& edge : edges_[u]) {
        if (edge.target == v) return edge.weight;
    }
    return default_val;
}

size_t BitsetAdjacencyList::common_neighbors(core::NodeId u, core::NodeId v) const {
    if (u >= num_nodes_ || v >= num_nodes_) return 0;

    // Fast check: zero overlap in bitmasks means no common neighbors exist
    uint64_t common_mask = bitmask_filters_[u] & bitmask_filters_[v];
    if (common_mask == 0) return 0;

    size_t count = 0;
    for (const auto& eu : edges_[u]) {
        if (has_edge(v, eu.target)) {
            count++;
        }
    }
    return count;
}

} // namespace graphflow::graph
