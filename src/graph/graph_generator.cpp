#include "graphflow/graph/graph_generator.hpp"
#include <random>
#include <algorithm>

using namespace std;

namespace graphflow::graph {

DynamicGraph GraphGenerator::generate_random_graph(
    size_t num_nodes,
    size_t num_edges,
    core::EdgeWeight min_weight,
    core::EdgeWeight max_weight,
    bool directed,
    uint64_t seed
) {
    DynamicGraph g(num_nodes, directed);
    if (num_nodes <= 1) return g;

    mt19937_64 rng(seed);
    uniform_int_distribution<core::NodeId> node_dist(0, static_cast<core::NodeId>(num_nodes - 1));
    uniform_real_distribution<core::EdgeWeight> weight_dist(min_weight, max_weight);

    // Build spanning tree backbone to guarantee global connectivity
    for (size_t i = 1; i < num_nodes; ++i) {
        core::NodeId parent = uniform_int_distribution<core::NodeId>(0, static_cast<core::NodeId>(i - 1))(rng);
        g.add_edge(parent, static_cast<core::NodeId>(i), weight_dist(rng));
    }

    size_t remaining = (num_edges > (num_nodes - 1)) ? (num_edges - (num_nodes - 1)) : 0;
    for (size_t i = 0; i < remaining; ++i) {
        core::NodeId u = node_dist(rng);
        core::NodeId v = node_dist(rng);
        if (u != v) {
            g.add_edge(u, v, weight_dist(rng));
        }
    }

    return g;
}

DynamicGraph GraphGenerator::generate_grid_graph(
    size_t width,
    size_t height,
    core::EdgeWeight min_weight,
    core::EdgeWeight max_weight,
    uint64_t seed
) {
    size_t total_nodes = width * height;
    DynamicGraph g(total_nodes, false);

    mt19937_64 rng(seed);
    uniform_real_distribution<core::EdgeWeight> weight_dist(min_weight, max_weight);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            core::NodeId u = static_cast<core::NodeId>(y * width + x);
            if (x + 1 < width) {
                core::NodeId v = static_cast<core::NodeId>(y * width + (x + 1));
                g.add_edge(u, v, weight_dist(rng));
            }
            if (y + 1 < height) {
                core::NodeId v = static_cast<core::NodeId>((y + 1) * width + x);
                g.add_edge(u, v, weight_dist(rng));
            }
        }
    }

    return g;
}

DynamicGraph GraphGenerator::generate_dag(
    size_t num_nodes,
    double edge_prob,
    core::EdgeWeight min_weight,
    core::EdgeWeight max_weight,
    uint64_t seed
) {
    DynamicGraph g(num_nodes, true);
    mt19937_64 rng(seed);
    uniform_real_distribution<double> prob_dist(0.0, 1.0);
    uniform_real_distribution<core::EdgeWeight> weight_dist(min_weight, max_weight);

    // Edges strictly from lower to higher indices guarantee DAG property
    for (size_t u = 0; u < num_nodes; ++u) {
        for (size_t v = u + 1; v < num_nodes; ++v) {
            if (prob_dist(rng) < edge_prob) {
                g.add_edge(static_cast<core::NodeId>(u), static_cast<core::NodeId>(v), weight_dist(rng));
            }
        }
    }

    return g;
}

DynamicGraph GraphGenerator::generate_flow_network(
    size_t num_layers,
    size_t nodes_per_layer,
    core::FlowType max_capacity,
    uint64_t seed
) {
    size_t total_nodes = 2 + num_layers * nodes_per_layer;
    DynamicGraph g(total_nodes, true);

    core::NodeId source = 0;
    core::NodeId sink = static_cast<core::NodeId>(total_nodes - 1);

    mt19937_64 rng(seed);
    uniform_int_distribution<core::FlowType> cap_dist(1, max_capacity);

    // Source to first layer
    for (size_t i = 0; i < nodes_per_layer; ++i) {
        core::NodeId u = static_cast<core::NodeId>(1 + i);
        g.add_flow_edge(source, u, cap_dist(rng));
    }

    // Intermediate layers
    for (size_t layer = 0; layer + 1 < num_layers; ++layer) {
        size_t l1_start = 1 + layer * nodes_per_layer;
        size_t l2_start = 1 + (layer + 1) * nodes_per_layer;
        for (size_t i = 0; i < nodes_per_layer; ++i) {
            for (size_t j = 0; j < nodes_per_layer; ++j) {
                if (uniform_int_distribution<int>(0, 1)(rng)) {
                    g.add_flow_edge(
                        static_cast<core::NodeId>(l1_start + i),
                        static_cast<core::NodeId>(l2_start + j),
                        cap_dist(rng)
                    );
                }
            }
        }
    }

    // Final layer to sink
    size_t last_layer_start = 1 + (num_layers - 1) * nodes_per_layer;
    for (size_t i = 0; i < nodes_per_layer; ++i) {
        core::NodeId u = static_cast<core::NodeId>(last_layer_start + i);
        g.add_flow_edge(u, sink, cap_dist(rng));
    }

    return g;
}

BitsetAdjacencyList GraphGenerator::generate_benchmark_bitset_graph(
    size_t num_nodes,
    size_t avg_degree,
    float min_weight,
    float max_weight,
    uint64_t seed
) {
    BitsetAdjacencyList g(num_nodes);
    mt19937_64 rng(seed);
    uniform_int_distribution<core::NodeId> node_dist(0, static_cast<core::NodeId>(num_nodes - 1));
    uniform_real_distribution<float> weight_dist(min_weight, max_weight);

    // Connected ring backbone + random chords
    for (size_t i = 0; i < num_nodes; ++i) {
        core::NodeId next = static_cast<core::NodeId>((i + 1) % num_nodes);
        g.add_edge(static_cast<core::NodeId>(i), next, weight_dist(rng));
    }

    size_t extra_edges_per_node = (avg_degree > 1) ? (avg_degree - 1) : 1;
    for (size_t i = 0; i < num_nodes; ++i) {
        for (size_t d = 0; d < extra_edges_per_node; ++d) {
            core::NodeId target = node_dist(rng);
            if (target != i) {
                g.add_edge(static_cast<core::NodeId>(i), target, weight_dist(rng));
            }
        }
    }

    return g;
}

} // namespace graphflow::graph
