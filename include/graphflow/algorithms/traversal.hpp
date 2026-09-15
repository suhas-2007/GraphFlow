#pragma once

#include <vector>
#include <queue>
#include <stack>
#include <functional>
#include "graphflow/core/types.hpp"
#include "graphflow/core/bitset.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

/**
 * @brief High-performance Graph Traversals and Bit-Parallel Search.
 */
class Traversal {
public:
    /**
     * @brief Standard Breadth-First Search visitor.
     */
    template <typename Visitor>
    static void bfs(const graph::DynamicGraph& g, core::NodeId start, Visitor&& visitor) {
        if (start >= g.num_nodes()) return;

        core::DynamicBitset visited(g.num_nodes(), false);
        std::queue<core::NodeId> q;

        visited.set(start);
        q.push(start);

        while (!q.empty()) {
            core::NodeId u = q.front();
            q.pop();

            visitor(u);

            for (const auto& edge : g.out_edges(u)) {
                if (!visited.test(edge.target)) {
                    visited.set(edge.target);
                    q.push(edge.target);
                }
            }
        }
    }

    /**
     * @brief High-throughput Multi-Source Bit-Parallel BFS.
     * Computes reachability from a set of source nodes concurrently using bitset frontiers.
     */
    static core::DynamicBitset multi_source_bfs(
        const graph::DynamicGraph& g,
        const std::vector<core::NodeId>& sources,
        size_t max_depth = std::numeric_limits<size_t>::max()
    ) {
        core::DynamicBitset visited(g.num_nodes(), false);
        core::DynamicBitset frontier(g.num_nodes(), false);

        for (core::NodeId s : sources) {
            if (s < g.num_nodes()) {
                visited.set(s);
                frontier.set(s);
            }
        }

        size_t depth = 0;
        while (!frontier.none() && depth < max_depth) {
            core::DynamicBitset next_frontier(g.num_nodes(), false);

            frontier.for_each_set_bit([&](size_t u) {
                for (const auto& edge : g.out_edges(static_cast<core::NodeId>(u))) {
                    if (!visited.test(edge.target)) {
                        visited.set(edge.target);
                        next_frontier.set(edge.target);
                    }
                }
            });

            frontier = std::move(next_frontier);
            depth++;
        }

        return visited;
    }

    /**
     * @brief Kahn's algorithm for topological sorting of DAGs.
     * Returns empty vector if a cycle exists.
     */
    static std::vector<core::NodeId> topological_sort(const graph::DynamicGraph& g) {
        const size_t n = g.num_nodes();
        std::vector<uint32_t> in_degree(n, 0);

        for (size_t u = 0; u < n; ++u) {
            for (const auto& edge : g.out_edges(static_cast<core::NodeId>(u))) {
                if (edge.target < n) {
                    in_degree[edge.target]++;
                }
            }
        }

        std::queue<core::NodeId> q;
        for (size_t i = 0; i < n; ++i) {
            if (in_degree[i] == 0) {
                q.push(static_cast<core::NodeId>(i));
            }
        }

        std::vector<core::NodeId> order;
        order.reserve(n);

        while (!q.empty()) {
            core::NodeId u = q.front();
            q.pop();
            order.push_back(u);

            for (const auto& edge : g.out_edges(u)) {
                if (--in_degree[edge.target] == 0) {
                    q.push(edge.target);
                }
            }
        }

        if (order.size() != n) {
            return {}; // Graph has cycles
        }
        return order;
    }
};

} // namespace graphflow::algorithms
