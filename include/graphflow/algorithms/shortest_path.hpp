#pragma once

#include <vector>
#include <functional>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"

namespace graphflow::algorithms {

using namespace core;
using graph::DynamicGraph;
using graph::BitsetAdjacencyList;

class ShortestPath {
public:
    static PathResult dijkstra_std(
        const DynamicGraph& g,
        NodeId source,
        NodeId target = kInvalidNode
    );

    static PathResult dijkstra_indexed_4ary(
        const DynamicGraph& g,
        NodeId source,
        NodeId target = kInvalidNode
    );

    static PathResult dijkstra_bitset(
        const BitsetAdjacencyList& g,
        NodeId source,
        NodeId target = kInvalidNode
    );

    static PathResult a_star(
        const DynamicGraph& g,
        NodeId source,
        NodeId target,
        std::function<EdgeWeight(NodeId, NodeId)> heuristic
    );

    static PathResult bidirectional_dijkstra(
        const DynamicGraph& g,
        NodeId source,
        NodeId target
    );
};

} // namespace graphflow::algorithms
