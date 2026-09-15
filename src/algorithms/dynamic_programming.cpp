#include "graphflow/algorithms/dynamic_programming.hpp"
#include "graphflow/algorithms/traversal.hpp"
#include <algorithm>
#include <queue>
#include <optional>

using namespace std;
using namespace graphflow::core;
using namespace graphflow::graph;

namespace graphflow::algorithms {

optional<CriticalPathResult> DynamicProgramming::critical_path_method(const DynamicGraph& dag) {
    const size_t n = dag.num_nodes();
    if (n == 0) return nullopt;

    auto topo_order = Traversal::topological_sort(dag);
    if (topo_order.empty()) {
        return nullopt; // Cycle detected; CPM requires a DAG
    }

    vector<EdgeWeight> earliest(n, 0.0);
    vector<NodeId> parent(n, kInvalidNode);

    for (NodeId u : topo_order) {
        for (const auto& edge : dag.out_edges(u)) {
            NodeId v = edge.target;
            if (earliest[u] + edge.weight > earliest[v]) {
                earliest[v] = earliest[u] + edge.weight;
                parent[v] = u;
            }
        }
    }

    NodeId max_node = 0;
    EdgeWeight max_dist = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (earliest[i] > max_dist) {
            max_dist = earliest[i];
            max_node = static_cast<NodeId>(i);
        }
    }

    vector<EdgeWeight> latest(n, max_dist);
    for (auto it = topo_order.rbegin(); it != topo_order.rend(); ++it) {
        NodeId u = *it;
        for (const auto& edge : dag.out_edges(u)) {
            NodeId v = edge.target;
            latest[u] = min(latest[u], latest[v] - edge.weight);
        }
    }

    vector<EdgeWeight> slack(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        slack[i] = max(0.0, latest[i] - earliest[i]);
    }

    vector<NodeId> crit_path;
    for (NodeId cur = max_node; cur != kInvalidNode; cur = parent[cur]) {
        crit_path.push_back(cur);
    }
    reverse(crit_path.begin(), crit_path.end());

    return CriticalPathResult{
        .critical_path_length = max_dist,
        .critical_path = move(crit_path),
        .earliest_start = move(earliest),
        .latest_start = move(latest),
        .slack = move(slack)
    };
}

uint64_t DynamicProgramming::count_paths_dag(
    const DynamicGraph& dag,
    NodeId source,
    NodeId target
) {
    const size_t n = dag.num_nodes();
    if (source >= n || target >= n) return 0;

    auto topo_order = Traversal::topological_sort(dag);
    if (topo_order.empty()) return 0;

    vector<uint64_t> dp(n, 0);
    dp[source] = 1;

    for (NodeId u : topo_order) {
        if (dp[u] == 0) continue;
        for (const auto& edge : dag.out_edges(u)) {
            dp[edge.target] += dp[u];
        }
    }

    return dp[target];
}

TspResult DynamicProgramming::solve_tsp_bitmask(
    const DynamicGraph& g,
    NodeId start_node
) {
    const size_t n = g.num_nodes();
    if (n == 0 || n > 24) {
        return {.min_cost = kInfinityWeight, .tour = {}};
    }
    if (n == 1) {
        return {.min_cost = 0.0, .tour = {start_node}};
    }

    const uint32_t num_states = 1 << n;
    vector<vector<EdgeWeight>> dp(num_states, vector<EdgeWeight>(n, kInfinityWeight));
    vector<vector<NodeId>> parent(num_states, vector<NodeId>(n, kInvalidNode));

    dp[1 << start_node][start_node] = 0.0;

    for (uint32_t mask = 1; mask < num_states; ++mask) {
        for (size_t u = 0; u < n; ++u) {
            if (!(mask & (1 << u))) continue;
            if (dp[mask][u] >= kInfinityWeight) continue;

            for (const auto& edge : g.out_edges(static_cast<NodeId>(u))) {
                NodeId v = edge.target;
                if (mask & (1 << v)) continue;

                uint32_t next_mask = mask | (1 << v);
                EdgeWeight next_cost = dp[mask][u] + edge.weight;
                if (next_cost < dp[next_mask][v]) {
                    dp[next_mask][v] = next_cost;
                    parent[next_mask][v] = static_cast<NodeId>(u);
                }
            }
        }
    }

    uint32_t final_mask = num_states - 1;
    EdgeWeight best_cost = kInfinityWeight;
    NodeId best_last = kInvalidNode;

    for (size_t u = 0; u < n; ++u) {
        if (dp[final_mask][u] < kInfinityWeight) {
            for (const auto& edge : g.out_edges(static_cast<NodeId>(u))) {
                if (edge.target == start_node) {
                    EdgeWeight total = dp[final_mask][u] + edge.weight;
                    if (total < best_cost) {
                        best_cost = total;
                        best_last = static_cast<NodeId>(u);
                    }
                }
            }
        }
    }

    if (best_last == kInvalidNode) {
        return {.min_cost = kInfinityWeight, .tour = {}};
    }

    vector<NodeId> tour;
    tour.push_back(start_node);
    uint32_t cur_mask = final_mask;
    NodeId cur_u = best_last;

    while (cur_u != start_node && cur_u != kInvalidNode) {
        tour.push_back(cur_u);
        NodeId p = parent[cur_mask][cur_u];
        cur_mask ^= (1 << cur_u);
        cur_u = p;
    }
    tour.push_back(start_node);
    reverse(tour.begin(), tour.end());

    return {.min_cost = best_cost, .tour = move(tour)};
}

PathResult DynamicProgramming::constrained_shortest_path(
    const DynamicGraph& g,
    const vector<vector<uint32_t>>& edge_resource_costs,
    NodeId source,
    NodeId target,
    uint32_t max_resource
) {
    const size_t n = g.num_nodes();
    if (source >= n || target >= n) {
        return {.distance = kInfinityWeight};
    }

    vector<vector<EdgeWeight>> dp(n, vector<EdgeWeight>(max_resource + 1, kInfinityWeight));
    vector<vector<pair<NodeId, uint32_t>>> parent(
        n, vector<pair<NodeId, uint32_t>>(max_resource + 1, {kInvalidNode, 0})
    );

    struct State {
        EdgeWeight dist;
        NodeId node;
        uint32_t resource;
        bool operator>(const State& o) const { return dist > o.dist; }
    };

    priority_queue<State, vector<State>, greater<State>> pq;

    dp[source][0] = 0.0;
    pq.push({0.0, source, 0});

    uint64_t visited_states = 0;

    while (!pq.empty()) {
        auto [d, u, r] = pq.top();
        pq.pop();

        if (d > dp[u][r]) continue;
        visited_states++;

        if (u == target) {
            vector<NodeId> path;
            NodeId cur_node = target;
            uint32_t cur_r = r;

            while (cur_node != kInvalidNode) {
                path.push_back(cur_node);
                if (cur_node == source && cur_r == 0) break;
                auto [p_node, p_r] = parent[cur_node][cur_r];
                cur_node = p_node;
                cur_r = p_r;
            }
            reverse(path.begin(), path.end());

            return {
                .distance = d,
                .path = move(path),
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
                EdgeWeight next_dist = d + edge.weight;
                if (next_dist < dp[edge.target][next_r]) {
                    dp[edge.target][next_r] = next_dist;
                    parent[edge.target][next_r] = {u, r};
                    pq.push({next_dist, edge.target, next_r});
                }
            }
        }
    }

    return {.distance = kInfinityWeight, .nodes_visited = visited_states};
}

} // namespace graphflow::algorithms
