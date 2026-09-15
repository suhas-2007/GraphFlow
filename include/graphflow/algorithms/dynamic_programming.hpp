#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

using namespace core;
using graph::DynamicGraph;

struct CriticalPathResult {
    EdgeWeight critical_path_length{0.0};
    std::vector<NodeId> critical_path;
    std::vector<EdgeWeight> earliest_start;
    std::vector<EdgeWeight> latest_start;
    std::vector<EdgeWeight> slack;
};

struct TspResult {
    EdgeWeight min_cost{kInfinityWeight};
    std::vector<NodeId> tour;
};

class DynamicProgramming {
public:
    static std::optional<CriticalPathResult> critical_path_method(const DynamicGraph& dag);

    static uint64_t count_paths_dag(
        const DynamicGraph& dag,
        NodeId source,
        NodeId target
    );

    static TspResult solve_tsp_bitmask(
        const DynamicGraph& g,
        NodeId start_node = 0
    );

    static PathResult constrained_shortest_path(
        const DynamicGraph& g,
        const std::vector<std::vector<uint32_t>>& edge_resource_costs,
        NodeId source,
        NodeId target,
        uint32_t max_resource
    );
};

} // namespace graphflow::algorithms
