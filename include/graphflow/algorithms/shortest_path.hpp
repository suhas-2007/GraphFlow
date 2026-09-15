#pragma once

#include <vector>
#include <functional>
#include "graphflow/core/types.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"

namespace graphflow::algorithms {

class ShortestPath {
public:
    /**
     * @brief Baseline Dijkstra implementation using std::priority_queue.
     * Experiences O(E log V) memory growth from duplicate entries.
     */
    static core::PathResult dijkstra_std(
        const graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId target = core::kInvalidNode
    );

    /**
     * @brief GraphFlow Accelerated Dijkstra using 4-ary Indexed Min-Heap.
     * Guarantees strictly <= V entries and in-place decrease_key.
     */
    static core::PathResult dijkstra_indexed_4ary(
        const graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId target = core::kInvalidNode
    );

    /**
     * @brief GraphFlow Cache-Optimized Pathfinding across Bitset Adjacency Lists.
     * Evaluates compact float edges with 4-ary indexed heap (powers 500k+ nodes benchmark).
     */
    static core::PathResult dijkstra_bitset(
        const graph::BitsetAdjacencyList& g,
        core::NodeId source,
        core::NodeId target = core::kInvalidNode
    );

    /**
     * @brief A* Search Algorithm using 4-ary Indexed Min-Heap and custom heuristic.
     */
    static core::PathResult a_star(
        const graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId target,
        std::function<core::EdgeWeight(core::NodeId, core::NodeId)> heuristic
    );

    /**
     * @brief Bidirectional Dijkstra: simultaneous forward and backward search.
     * Significantly reduces searched state space.
     */
    static core::PathResult bidirectional_dijkstra(
        const graph::DynamicGraph& g,
        core::NodeId source,
        core::NodeId target
    );
};

} // namespace graphflow::algorithms
