#include "graphflow/algorithms/max_flow.hpp"
#include <queue>
#include <algorithm>
#include <chrono>
#include <vector>
#include <limits>

using namespace std;

namespace graphflow::algorithms {

namespace {

bool dinic_bfs(
    graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId sink,
    vector<int>& level
) {
    fill(level.begin(), level.end(), -1);
    queue<core::NodeId> q;

    level[source] = 0;
    q.push(source);

    while (!q.empty()) {
        core::NodeId u = q.front();
        q.pop();

        for (const auto& edge : g.flow_edges(u)) {
            if (edge.residual_capacity() > 0 && level[edge.to] == -1) {
                level[edge.to] = level[u] + 1;
                q.push(edge.to);
            }
        }
    }

    return level[sink] != -1;
}

core::FlowType dinic_dfs(
    graph::DynamicGraph& g,
    core::NodeId u,
    core::NodeId sink,
    core::FlowType pushed,
    const vector<int>& level,
    vector<size_t>& ptr
) {
    if (pushed == 0 || u == sink) return pushed;

    auto& edges = g.flow_edges(u);
    for (size_t& cid = ptr[u]; cid < edges.size(); ++cid) {
        auto& edge = edges[cid];
        core::NodeId v = edge.to;

        if (level[u] + 1 != level[v] || edge.residual_capacity() == 0) {
            continue;
        }

        core::FlowType tr = dinic_dfs(g, v, sink, min(pushed, edge.residual_capacity()), level, ptr);
        if (tr == 0) continue;

        edge.flow += tr;
        g.flow_edges(v)[edge.rev].flow -= tr;
        return tr;
    }

    return 0;
}

} // namespace

core::FlowResult MaxFlow::dinic(
    graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId sink
) {
    auto start_time = chrono::high_resolution_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || sink >= n || source == sink) {
        return {.max_flow = 0};
    }

    // Reset flows across all edges
    for (size_t i = 0; i < n; ++i) {
        for (auto& edge : g.flow_edges(static_cast<core::NodeId>(i))) {
            edge.flow = 0;
        }
    }

    vector<int> level(n);
    vector<size_t> ptr(n);
    core::FlowType max_flow = 0;

    while (dinic_bfs(g, source, sink, level)) {
        fill(ptr.begin(), ptr.end(), 0);
        while (core::FlowType pushed = dinic_dfs(g, source, sink, core::kInfinityFlow, level, ptr)) {
            max_flow += pushed;
        }
    }

    // Identify source-side min-cut partition
    vector<core::NodeId> min_cut;
    vector<bool> visited(n, false);
    queue<core::NodeId> q;
    visited[source] = true;
    q.push(source);

    while (!q.empty()) {
        core::NodeId u = q.front();
        q.pop();
        min_cut.push_back(u);

        for (const auto& edge : g.flow_edges(u)) {
            if (edge.residual_capacity() > 0 && !visited[edge.to]) {
                visited[edge.to] = true;
                q.push(edge.to);
            }
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end_time - start_time).count();

    return {
        .max_flow = max_flow,
        .min_cost = 0,
        .min_cut_source_side = move(min_cut),
        .execution_time_ms = ms
    };
}

core::FlowResult MaxFlow::push_relabel_hlpp(
    graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId sink
) {
    auto start_time = chrono::high_resolution_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || sink >= n || source == sink) {
        return {.max_flow = 0};
    }

    for (size_t i = 0; i < n; ++i) {
        for (auto& edge : g.flow_edges(static_cast<core::NodeId>(i))) {
            edge.flow = 0;
        }
    }

    vector<core::FlowType> excess(n, 0);
    vector<size_t> height(n, 0);
    vector<size_t> count(2 * n, 0);
    vector<vector<core::NodeId>> buckets(2 * n);
    size_t highest_active = 0;

    auto push_active = [&](core::NodeId u) {
        if (u != source && u != sink) {
            buckets[height[u]].push_back(u);
            highest_active = max(highest_active, height[u]);
        }
    };

    // Backward BFS from sink to initialize exact distance labels
    auto global_relabel = [&]() {
        fill(height.begin(), height.end(), n);
        fill(count.begin(), count.end(), 0);
        for (auto& b : buckets) b.clear();

        queue<core::NodeId> q;
        height[sink] = 0;
        q.push(sink);

        while (!q.empty()) {
            core::NodeId u = q.front();
            q.pop();

            for (const auto& edge : g.flow_edges(u)) {
                const auto& rev_edge = g.flow_edges(edge.to)[edge.rev];
                if (rev_edge.residual_capacity() > 0 && height[edge.to] == n) {
                    height[edge.to] = height[u] + 1;
                    q.push(edge.to);
                }
            }
        }

        for (size_t i = 0; i < n; ++i) {
            if (height[i] < n) {
                count[height[i]]++;
                if (excess[i] > 0 && i != source && i != sink) {
                    push_active(static_cast<core::NodeId>(i));
                }
            }
        }
    };

    // Saturate source outgoing edges
    height[source] = n;
    excess[source] = core::kInfinityFlow;
    excess[sink] = -core::kInfinityFlow;

    for (auto& edge : g.flow_edges(source)) {
        if (edge.capacity > 0) {
            core::FlowType f = edge.capacity;
            edge.flow += f;
            g.flow_edges(edge.to)[edge.rev].flow -= f;
            excess[edge.to] += f;
            excess[source] -= f;
            if (edge.to != sink) {
                push_active(edge.to);
            }
        }
    }

    global_relabel();

    size_t work = 0;
    const size_t work_limit = 4 * n;

    while (true) {
        while (highest_active > 0 && buckets[highest_active].empty()) {
            highest_active--;
        }
        if (buckets[highest_active].empty()) break;

        core::NodeId u = buckets[highest_active].back();
        buckets[highest_active].pop_back();

        // Discharge excess from node u
        while (excess[u] > 0) {
            size_t min_h = 2 * n;
            for (auto& edge : g.flow_edges(u)) {
                if (edge.residual_capacity() > 0) {
                    if (height[u] == height[edge.to] + 1) {
                        core::FlowType push_amt = min(excess[u], edge.residual_capacity());
                        edge.flow += push_amt;
                        g.flow_edges(edge.to)[edge.rev].flow -= push_amt;
                        excess[u] -= push_amt;
                        excess[edge.to] += push_amt;

                        if (excess[edge.to] == push_amt && edge.to != source && edge.to != sink) {
                            push_active(edge.to);
                        }
                        if (excess[u] == 0) break;
                    } else {
                        min_h = min(min_h, height[edge.to]);
                    }
                }
            }

            if (excess[u] > 0) {
                // Relabel node u
                size_t old_h = height[u];
                count[old_h]--;

                // Gap heuristic: empty level isolates all nodes above it
                if (count[old_h] == 0 && old_h < n) {
                    for (size_t i = 0; i < n; ++i) {
                        if (height[i] > old_h && height[i] < n) {
                            count[height[i]]--;
                            height[i] = n + 1;
                        }
                    }
                }

                height[u] = (min_h == 2 * n) ? n + 1 : min_h + 1;
                count[height[u]]++;
                highest_active = height[u];
            }
        }

        work++;
        if (work >= work_limit) {
            global_relabel();
            work = 0;
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end_time - start_time).count();

    core::FlowType total_flow = 0;
    for (const auto& edge : g.flow_edges(source)) {
        total_flow += edge.flow;
    }

    return {
        .max_flow = total_flow,
        .min_cost = 0,
        .execution_time_ms = ms
    };
}

core::FlowResult MaxFlow::min_cost_max_flow(
    graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId sink
) {
    auto start_time = chrono::high_resolution_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || sink >= n || source == sink) {
        return {.max_flow = 0, .min_cost = 0};
    }

    for (size_t i = 0; i < n; ++i) {
        for (auto& edge : g.flow_edges(static_cast<core::NodeId>(i))) {
            edge.flow = 0;
        }
    }

    core::FlowType flow = 0;
    core::CostType cost = 0;

    // Successive shortest path with SPFA
    while (true) {
        vector<core::CostType> dist(n, core::kInfinityCost);
        vector<core::NodeId> parent_node(n, core::kInvalidNode);
        vector<size_t> parent_edge_idx(n, 0);
        vector<bool> in_queue(n, false);
        queue<core::NodeId> q;

        dist[source] = 0;
        q.push(source);
        in_queue[source] = true;

        while (!q.empty()) {
            core::NodeId u = q.front();
            q.pop();
            in_queue[u] = false;

            const auto& edges = g.flow_edges(u);
            for (size_t i = 0; i < edges.size(); ++i) {
                const auto& edge = edges[i];
                if (edge.residual_capacity() > 0 && dist[u] + edge.cost < dist[edge.to]) {
                    dist[edge.to] = dist[u] + edge.cost;
                    parent_node[edge.to] = u;
                    parent_edge_idx[edge.to] = i;

                    if (!in_queue[edge.to]) {
                        q.push(edge.to);
                        in_queue[edge.to] = true;
                    }
                }
            }
        }

        if (dist[sink] == core::kInfinityCost) {
            break; // No augmenting path remains
        }

        // Bottleneck capacity along the path
        core::FlowType push_amt = core::kInfinityFlow;
        for (core::NodeId cur = sink; cur != source; cur = parent_node[cur]) {
            core::NodeId p = parent_node[cur];
            size_t edge_idx = parent_edge_idx[cur];
            push_amt = min(push_amt, g.flow_edges(p)[edge_idx].residual_capacity());
        }

        // Augment flow
        for (core::NodeId cur = sink; cur != source; cur = parent_node[cur]) {
            core::NodeId p = parent_node[cur];
            size_t edge_idx = parent_edge_idx[cur];
            auto& edge = g.flow_edges(p)[edge_idx];
            edge.flow += push_amt;
            g.flow_edges(cur)[edge.rev].flow -= push_amt;
            cost += push_amt * edge.cost;
        }

        flow += push_amt;
    }

    auto end_time = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end_time - start_time).count();

    return {
        .max_flow = flow,
        .min_cost = cost,
        .execution_time_ms = ms
    };
}

} // namespace graphflow::algorithms
