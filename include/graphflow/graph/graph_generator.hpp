#pragma once

#include <cstdint>
#include <cstddef>
#include <random>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"

namespace graphflow::graph {

using namespace core;

class GraphGenerator {
public:
    static DynamicGraph generate_random_graph(
        size_t num_nodes,
        size_t num_edges,
        EdgeWeight min_weight = 1.0,
        EdgeWeight max_weight = 100.0,
        bool directed = true,
        uint64_t seed = 42
    );

    static DynamicGraph generate_grid_graph(
        size_t width,
        size_t height,
        EdgeWeight min_weight = 1.0,
        EdgeWeight max_weight = 10.0,
        uint64_t seed = 42
    );

    static DynamicGraph generate_dag(
        size_t num_nodes,
        double edge_prob = 0.05,
        EdgeWeight min_weight = 1.0,
        EdgeWeight max_weight = 50.0,
        uint64_t seed = 42
    );

    static DynamicGraph generate_flow_network(
        size_t num_layers,
        size_t nodes_per_layer,
        FlowType max_capacity = 100,
        uint64_t seed = 42
    );

    static BitsetAdjacencyList generate_benchmark_bitset_graph(
        size_t num_nodes,
        size_t avg_degree,
        float min_weight = 1.0f,
        float max_weight = 50.0f,
        uint64_t seed = 42
    );
};

} // namespace graphflow::graph
