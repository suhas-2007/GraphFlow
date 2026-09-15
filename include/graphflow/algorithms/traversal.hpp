#pragma once

#include <vector>
#include <queue>
#include <limits>
#include <utility>
#include "graphflow/core/types.hpp"
#include "graphflow/core/bitset.hpp"
#include "graphflow/graph/graph.hpp"

namespace graphflow::algorithms {

using namespace core;
using graph::DynamicGraph;

class Traversal {
public:
    template <typename Visitor>
    static void bfs(const DynamicGraph& g, NodeId start, Visitor&& visitor) {
        if (start >= g.num_nodes()) return;

        DynamicBitset visited(g.num_nodes(), false);
        std::queue<NodeId> q;

        visited.set(start);
        q.push(start);

        while (!q.empty()) {
            NodeId u = q.front();
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

    static DynamicBitset multi_source_bfs(
        const DynamicGraph& g,
        const std::vector<NodeId>& sources,
        size_t max_depth = std::numeric_limits<size_t>::max()
    ) {
        DynamicBitset visited(g.num_nodes(), false);
        DynamicBitset frontier(g.num_nodes(), false);

        for (NodeId s : sources) {
            if (s < g.num_nodes()) {
                visited.set(s);
                frontier.set(s);
            }
        }

        size_t depth = 0;
        while (!frontier.none() && depth < max_depth) {
            DynamicBitset next_frontier(g.num_nodes(), false);

            frontier.for_each_set_bit([&](size_t u) {
                for (const auto& edge : g.out_edges(static_cast<NodeId>(u))) {
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

    static std::vector<NodeId> topological_sort(const DynamicGraph& g) {
        const size_t n = g.num_nodes();
        std::vector<uint32_t> in_degree(n, 0);

        for (size_t u = 0; u < n; ++u) {
            for (const auto& edge : g.out_edges(static_cast<NodeId>(u))) {
                if (edge.target < n) {
                    in_degree[edge.target]++;
                }
            }
        }

        std::queue<NodeId> q;
        for (size_t i = 0; i < n; ++i) {
            if (in_degree[i] == 0) {
                q.push(static_cast<NodeId>(i));
            }
        }

        std::vector<NodeId> order;
        order.reserve(n);

        while (!q.empty()) {
            NodeId u = q.front();
            q.pop();
            order.push_back(u);

            for (const auto& edge : g.out_edges(u)) {
                if (--in_degree[edge.target] == 0) {
                    q.push(edge.target);
                }
            }
        }

        if (order.size() != n) {
            return {}; // Cycle detected
        }
        return order;
    }
};

} // namespace graphflow::algorithms
