#pragma once

#include <vector>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

class MaxFlow {
public:
    /**
     * @brief Dinic's Algorithm for Maximum Network Flow.
     * Uses Level Graphs (BFS) and Blocking Flows (DFS with current-edge pointers).
     * Time Complexity: O(V^2 E), O(E sqrt(V)) on unit networks.
     */
    static core::FlowResult dinic(
        graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId sink
    );

    /**
     * @brief Highest-Label Preflow-Push (Push-Relabel) with Gap Heuristic
     * and Global Relabeling.
     * Superior performance on dense networks and complex capacity configurations.
     * Time Complexity: O(V^2 sqrt(E)).
     */
    static core::FlowResult push_relabel_hlpp(
        graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId sink
    );

    /**
     * @brief Successive Shortest Path (SSP) for Minimum-Cost Maximum-Flow.
     * Augments flow along min-cost residual paths using potential-based SPFA.
     */
    static core::FlowResult min_cost_max_flow(
        graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId sink
    );
};

} // namespace graphflow::algorithms
