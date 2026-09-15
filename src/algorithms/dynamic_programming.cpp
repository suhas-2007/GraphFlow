#include "graphflow/algorithms/dynamic_programming.hpp"
#include "graphflow/algorithms/traversal.hpp"
#include <algorithm>
#include <queue>

namespace graphflow::algorithms {

std::optional<CriticalPathResult> DynamicProgramming::critical_path_method(const graph::DynamicGraph& dag) {
    const size_t n = dag.num_nodes();
    if (n == 0) return std::nullopt;

    auto topo_order = Traversal::topological_sort(dag);
    if (topo_order.empty()) {
        return std::nullopt; // Not a DAG
    }

    std::vector<core::EdgeWeight> earliest(n, 0.0);
    std::vector<core::NodeId> parent(n, core::kInvalidNode);

    // Forward pass: Earliest Start Time
    for (core::NodeId u : topo_order) {
        for (const auto& edge : dag.out_edges(u)) {
            core::NodeId v = edge.target;
            if (earliest[u] + edge.weight > earliest[v]) {
                earliest[v] = earliest[u] + edge.weight;
                parent[v] = u;
            }
        }
    }

    // Find sink node with maximum earliest time
    core::NodeId max_node = 0;
    core::EdgeWeight max_dist = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (earliest[i] > max_dist) {
            max_dist = earliest[i];
            max_node = static_cast<core::NodeId>(i);
        }
    }

    // Backward pass: Latest Start Time
    std::vector<core::EdgeWeight> latest(n, max_dist);
    for (auto it = topo_order.rbegin(); it != topo_order.rend(); ++it) {
        core::NodeId u = *it;
        for (const auto& edge : dag.out_edges(u)) {
            core::NodeId v = edge.target;
            latest[u] = std::min(latest[u], latest[v] - edge.weight);
        }
    }

    // Slack
    std::vector<core::EdgeWeight> slack(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        slack[i] = std::max(0.0, latest[i] - earliest[i]);
    }

    // Trace critical path
    std::vector<core::NodeId> crit_path;
    for (core::NodeId cur = max_node; cur != core::kInvalidNode; cur = parent[cur]) {
        crit_path.push_back(cur);
    }
    std::reverse(crit_path.begin(), crit_path.end());

    return CriticalPathResult{
        .critical_path_length = max_dist,
        .critical_path = std::move(crit_path),
        .earliest_start = std::move(earliest),
        .latest_start = std::move(latest),
        .slack = std::move(slack)
    };
}

uint64_t DynamicProgramming::count_paths_dag(
    const graph::DynamicGraph& dag,
    core::NodeId source,
    core::NodeId target
) {
    const size_t n = dag.num_nodes();
    if (source >= n || target >= n) return 0;

    auto topo_order = Traversal::topological_sort(dag);
    if (topo_order.empty()) return 0;

    std::vector<uint64_t> dp(n, 0);
    dp[source] = 1;

    for (core::NodeId u : topo_order) {
        if (dp[u] == 0) continue;
        for (const auto& edge : dag.out_edges(u)) {
            dp[edge.target] += dp[u];
        }
    }

    return dp[target];
}

TspResult DynamicProgramming::solve_tsp_bitmask(
    const graph::DynamicGraph& g,
    core::NodeId start_node
) {
    const size_t n = g.num_nodes();
    if (n == 0 || n > 24) {
        // Exceeds bitmask state space limit
        return {.min_cost = core::kInfinityWeight, .tour = {}};
    }
    if (n == 1) {
        return {.min_cost = 0.0, .tour = {start_node}};
    }

    const uint32_t num_states = 1 << n;
    // dp[mask][u]: min cost to visit states in mask ending at u
    std::vector<std::vector<core::EdgeWeight>> dp(num_states, std::vector<core::EdgeWeight>(n, core::kInfinityWeight));
    std::vector<std::vector<core::NodeId>> parent(num_states, std::vector<core::NodeId>(n, core::kInvalidNode));

    dp[1 << start_node][start_node] = 0.0;

    for (uint32_t mask = 1; mask < num_states; ++mask) {
        for (size_t u = 0; u < n; ++u) {
            if (!(mask & (1 << u))) continue;
            if (dp[mask][u] >= core::kInfinityWeight) continue;

            for (const auto& edge : g.out_edges(static_cast<core::NodeId>(u))) {
                core::NodeId v = edge.target;
                if (mask & (1 << v)) continue; // Already visited

                uint32_t next_mask = mask | (1 << v);
                core::EdgeWeight next_cost = dp[mask][u] + edge.weight;
                if (next_cost < dp[next_mask][v]) {
                    dp[next_mask][v] = next_cost;
                    parent[next_mask][v] = static_cast<core::NodeId>(u);
                }
            }
        }
    }

    // Find minimum cost to return to start_node
    uint32_t final_mask = num_states - 1;
    core::EdgeWeight best_cost = core::kInfinityWeight;
    core::NodeId best_last = core::kInvalidNode;

    for (size_t u = 0; u < n; ++u) {
        if (dp[final_mask][u] < core::kInfinityWeight) {
            for (const auto& edge : g.out_edges(static_cast<core::NodeId>(u))) {
                if (edge.target == start_node) {
                    core::EdgeWeight total = dp[final_mask][u] + edge.weight;
                    if (total < best_cost) {
                        best_cost = total;
                        best_last = static_cast<core::NodeId>(u);
                    }
                }
            }
        }
    }

    if (best_last == core::kInvalidNode) {
        return {.min_cost = core::kInfinityWeight, .tour = {}};
    }

    // Reconstruct tour
    std::vector<core::NodeId> tour;
    tour.push_back(start_node);
    uint32_t cur_mask = final_mask;
    core::NodeId cur_u = best_last;

    while (cur_u != start_node && cur_u != core::kInvalidNode) {
        tour.push_back(cur_u);
        core::NodeId p = parent[cur_mask][cur_u];
        cur_mask ^= (1 << cur_u);
        cur_u = p;
    }
    tour.push_back(start_node);
    std::reverse(tour.begin(), tour.end());

    return {.min_cost = best_cost, .tour = std::move(tour)};
}

core::PathResult DynamicProgramming::constrained_shortest_path(
    const graph::DynamicGraph& g,
    const std::vector<std::vector<uint32_t>>& edge_resource_costs,
    core::NodeId source,
    core::NodeId target,
    uint32_t max_resource
) {
    const size_t n = g.num_nodes();
    if (source >= n || target >= n) {
        return {.distance = core::kInfinityWeight};
    }

    // dp[u][r]: min distance to reach node u with accumulated resource r
    std::vector<std::vector<core::EdgeWeight>> dp(n, std::vector<core::EdgeWeight>(max_resource + 1, core::kInfinityWeight));
    std::vector<std::vector<std::pair<core::NodeId, uint32_t>>> parent(
        n, std::vector<std::pair<core::NodeId, uint32_t>>(max_resource + 1, {core::kInvalidNode, 0})
    );

    // Min-priority queue ordered by distance: <distance, node, resource>
    struct State {
        core::EdgeWeight dist;
        core::NodeId node;
        uint32_t resource;
        bool operator>(const State& o) const { return dist > o.dist; }
    };

    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;

    dp[source][0] = 0.0;
    pq.push({0.0, source, 0});

    uint64_t visited_states = 0;

    while (!pq.empty()) {
        auto [d, u, r] = pq.top();
        pq.pop();

        if (d > dp[u][r]) continue;
        visited_states++;

        if (u == target) {
            // Reconstruct path
            std::vector<core::NodeId> path;
            core::NodeId cur_node = target;
            uint32_t cur_r = r;

            while (cur_node != core::kInvalidNode) {
                path.push_back(cur_node);
                if (cur_node == source && cur_r == 0) break;
                auto [p_node, p_r] = parent[cur_node][cur_r];
                cur_node = p_node;
                cur_r = p_r;
            }
            std::reverse(path.begin(), path.end());

            return {
                .distance = d,
                .path = std::move(path),
                .nodes_visited = visited_states,
                .execution_time_ms = 0.0
            };
        }

        const auto& edges = g.out_edges(u);
        for (size_t i = 0; i < edges.size(); ++i) {
            const auto& edge = edges[i];
            uint32_t res_cost = (u < edge_resource_costs.size() && i < edge_resource_costs[u].size())
                                    ? edge_resource_costs[u][i] : 1;
            uint32_t next_r = r + res_cost;

            if (next_r <= max_resource) {
                core::EdgeWeight next_dist = d + edge.weight;
                if (next_dist < dp[edge.target][next_r]) {
                    dp[edge.target][next_r] = next_dist;
                    parent[edge.target][next_r] = {u, r};
                    pq.push({next_dist, edge.target, next_r});
                }
            }
        }
    }

    return {.distance = core::kInfinityWeight, .nodes_visited = visited_states};
}

} // namespace graphflow::algorithms
