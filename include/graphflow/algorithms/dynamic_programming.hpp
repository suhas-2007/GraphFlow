#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

struct CriticalPathResult {
    core::EdgeWeight critical_path_length{0.0};
    std::vector<core::NodeId> critical_path;
    std::vector<core::EdgeWeight> earliest_start;
    std::vector<core::EdgeWeight> latest_start;
    std::vector<core::EdgeWeight> slack;
};

struct TspResult {
    core::EdgeWeight min_cost{core::kInfinityWeight};
    std::vector<core::NodeId> tour;
};

class DynamicProgramming {
public:
    /**
     * @brief Computes Longest Path and Critical Path Method (CPM) on a DAG.
     * Essential for state-space scheduling, execution pipeline bottlenecks, and event graphs.
     */
    static std::optional<CriticalPathResult> critical_path_method(const graph::DynamicGraph& dag);

    /**
     * @brief Counts distinct paths between source and target on a DAG.
     */
    static uint64_t count_paths_dag(
        const graph::DynamicGraph& dag,
        core::NodeId source,
        core::NodeId target
    );

    /**
     * @brief Exact State-Space Subset Dynamic Programming (Bitmask DP).
     * Solves Traveling Salesperson / Hamiltonian cycle across N states (N <= 24) in O(N^2 * 2^N).
     */
    static TspResult solve_tsp_bitmask(
        const graph::DynamicGraph& g,
        core::NodeId start_node = 0
    );

    /**
     * @brief Resource-Constrained Shortest Path (RCSP) on state-space graphs.
     * Minimizes distance subject to edge traversal resource cost <= max_resource.
     */
    static core::PathResult constrained_shortest_path(
        const graph::DynamicGraph& g,
        const std::vector<std::vector<uint32_t>>& edge_resource_costs,
        core::NodeId source,
        core::NodeId target,
        uint32_t max_resource
    );
};

} // namespace graphflow::algorithms
