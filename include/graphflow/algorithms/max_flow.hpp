#pragma once

#include <vector>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

using namespace core;
using graph::DynamicGraph;

class MaxFlow {
public:
    static FlowResult dinic(
        DynamicGraph& g,
        NodeId source,
        NodeId sink
    );

    static FlowResult push_relabel_hlpp(
        DynamicGraph& g,
        NodeId source,
        NodeId sink
    );

    static FlowResult min_cost_max_flow(
        DynamicGraph& g,
        NodeId source,
        NodeId sink
    );
};

} // namespace graphflow::algorithms
