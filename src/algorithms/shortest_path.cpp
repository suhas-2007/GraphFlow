#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/core/dary_heap.hpp"
#include <queue>
#include <chrono>
#include <algorithm>

namespace graphflow::algorithms {

namespace {

std::vector<core::NodeId> reconstruct_path(
    const std::vector<core::NodeId>& parent,
    core::NodeId source,
    core::NodeId target
) {
    if (target == core::kInvalidNode || parent[target] == core::kInvalidNode && target != source) {
        return {};
    }

    std::vector<core::NodeId> path;
    for (core::NodeId cur = target; cur != core::kInvalidNode; cur = parent[cur]) {
        path.push_back(cur);
        if (cur == source) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace

core::PathResult ShortestPath::dijkstra_std(
    const graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId target
) {
    auto start_time = std::chrono::steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = core::kInfinityWeight};
    }

    std::vector<core::EdgeWeight> dist(n, core::kInfinityWeight);
    std::vector<core::NodeId> parent(n, core::kInvalidNode);
    std::vector<bool> visited(n, false);

    // Standard std::priority_queue (stores pairs of <distance, node>)
    using QueueElement = std::pair<core::EdgeWeight, core::NodeId>;
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;

    dist[source] = 0.0;
    pq.push({0.0, source});

    uint64_t nodes_visited = 0;

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;
        nodes_visited++;

        if (u == target) break;

        for (const auto& edge : g.out_edges(u)) {
            core::NodeId v = edge.target;
            core::EdgeWeight new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                pq.push({new_dist, v}); // Duplicate push in std::priority_queue
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    core::PathResult res;
    res.distance = (target != core::kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

core::PathResult ShortestPath::dijkstra_indexed_4ary(
    const graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId target
) {
    auto start_time = std::chrono::steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = core::kInfinityWeight};
    }

    std::vector<core::EdgeWeight> dist(n, core::kInfinityWeight);
    std::vector<core::NodeId> parent(n, core::kInvalidNode);

    core::IndexedDaryHeap<core::NodeId, core::EdgeWeight, 4> heap(n);

    dist[source] = 0.0;
    heap.push(source, 0.0);

    uint64_t nodes_visited = 0;

    while (!heap.empty()) {
        auto min_entry = heap.pop();
        core::NodeId u = min_entry.key;
        core::EdgeWeight d = min_entry.priority;

        nodes_visited++;
        if (u == target) break;

        for (const auto& edge : g.out_edges(u)) {
            core::NodeId v = edge.target;
            core::EdgeWeight new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                // In-place decrease_key with 4-ary tree traversal!
                heap.push_or_decrease_key(v, new_dist);
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    core::PathResult res;
    res.distance = (target != core::kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

core::PathResult ShortestPath::dijkstra_bitset(
    const graph::BitsetAdjacencyList& g,
    core::NodeId source,
    core::NodeId target
) {
    auto start_time = std::chrono::steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = core::kInfinityWeight};
    }

    std::vector<float> dist(n, static_cast<float>(core::kInfinityWeight));
    std::vector<core::NodeId> parent(n, core::kInvalidNode);

    core::IndexedDaryHeap<core::NodeId, float, 4> heap(n);

    dist[source] = 0.0f;
    heap.push(source, 0.0f);

    uint64_t nodes_visited = 0;

    while (!heap.empty()) {
        auto min_entry = heap.pop();
        core::NodeId u = min_entry.key;
        float d = min_entry.priority;

        nodes_visited++;
        if (u == target) break;

        for (const auto& edge : g.neighbors(u)) {
            core::NodeId v = edge.target;
            float new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                heap.push_or_decrease_key(v, new_dist);
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    core::PathResult res;
    res.distance = (target != core::kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

core::PathResult ShortestPath::a_star(
    const graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId target,
    std::function<core::EdgeWeight(core::NodeId, core::NodeId)> heuristic
) {
    auto start_time = std::chrono::steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || target >= n) {
        return {.distance = core::kInfinityWeight};
    }

    std::vector<core::EdgeWeight> g_score(n, core::kInfinityWeight);
    std::vector<core::NodeId> parent(n, core::kInvalidNode);

    core::IndexedDaryHeap<core::NodeId, core::EdgeWeight, 4> open_set(n);

    g_score[source] = 0.0;
    open_set.push(source, heuristic(source, target));

    uint64_t nodes_visited = 0;

    while (!open_set.empty()) {
        auto min_entry = open_set.pop();
        core::NodeId u = min_entry.key;
        nodes_visited++;

        if (u == target) break;

        for (const auto& edge : g.out_edges(u)) {
            core::NodeId v = edge.target;
            core::EdgeWeight tentative_g = g_score[u] + edge.weight;

            if (tentative_g < g_score[v]) {
                parent[v] = u;
                g_score[v] = tentative_g;
                core::EdgeWeight f_score = tentative_g + heuristic(v, target);
                open_set.push_or_decrease_key(v, f_score);
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    core::PathResult res;
    res.distance = g_score[target];
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

core::PathResult ShortestPath::bidirectional_dijkstra(
    const graph::DynamicGraph& g,
    core::NodeId source,
    core::NodeId target
) {
    auto start_time = std::chrono::steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || target >= n) {
        return {.distance = core::kInfinityWeight};
    }
    if (source == target) {
        return {.distance = 0.0, .path = {source}, .nodes_visited = 1, .execution_time_ms = 0.0};
    }

    std::vector<core::EdgeWeight> dist_f(n, core::kInfinityWeight);
    std::vector<core::EdgeWeight> dist_b(n, core::kInfinityWeight);
    std::vector<core::NodeId> parent_f(n, core::kInvalidNode);
    std::vector<core::NodeId> parent_b(n, core::kInvalidNode);
    std::vector<bool> visited_f(n, false);
    std::vector<bool> visited_b(n, false);

    core::IndexedDaryHeap<core::NodeId, core::EdgeWeight, 4> heap_f(n);
    core::IndexedDaryHeap<core::NodeId, core::EdgeWeight, 4> heap_b(n);

    // Build reverse adjacency for backward search
    std::vector<std::vector<core::Edge>> rev_adj(n);
    for (size_t u = 0; u < n; ++u) {
        for (const auto& e : g.out_edges(static_cast<core::NodeId>(u))) {
            rev_adj[e.target].push_back({static_cast<core::NodeId>(u), e.weight});
        }
    }

    dist_f[source] = 0.0;
    heap_f.push(source, 0.0);

    dist_b[target] = 0.0;
    heap_b.push(target, 0.0);

    core::EdgeWeight best_dist = core::kInfinityWeight;
    core::NodeId meeting_node = core::kInvalidNode;
    uint64_t nodes_visited = 0;

    while (!heap_f.empty() && !heap_b.empty()) {
        // Forward step
        if (!heap_f.empty()) {
            auto [u_f, d_f] = heap_f.pop();
            visited_f[u_f] = true;
            nodes_visited++;

            if (d_f + (heap_b.empty() ? 0.0 : heap_b.top().priority) >= best_dist) {
                break;
            }

            for (const auto& edge : g.out_edges(u_f)) {
                core::NodeId v = edge.target;
                core::EdgeWeight nd = d_f + edge.weight;
                if (nd < dist_f[v]) {
                    dist_f[v] = nd;
                    parent_f[v] = u_f;
                    heap_f.push_or_decrease_key(v, nd);
                }
                if (visited_b[v] && dist_f[u_f] + edge.weight + dist_b[v] < best_dist) {
                    best_dist = dist_f[u_f] + edge.weight + dist_b[v];
                    meeting_node = v;
                    parent_f[v] = u_f;
                }
            }
        }

        // Backward step
        if (!heap_b.empty()) {
            auto [u_b, d_b] = heap_b.pop();
            visited_b[u_b] = true;
            nodes_visited++;

            for (const auto& edge : rev_adj[u_b]) {
                core::NodeId v = edge.target;
                core::EdgeWeight nd = d_b + edge.weight;
                if (nd < dist_b[v]) {
                    dist_b[v] = nd;
                    parent_b[v] = u_b;
                    heap_b.push_or_decrease_key(v, nd);
                }
                if (visited_f[v] && dist_b[u_b] + edge.weight + dist_f[v] < best_dist) {
                    best_dist = dist_b[u_b] + edge.weight + dist_f[v];
                    meeting_node = v;
                    parent_b[v] = u_b;
                }
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    core::PathResult res;
    res.distance = best_dist;
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;

    if (meeting_node != core::kInvalidNode) {
        // Forward portion: source -> meeting_node
        std::vector<core::NodeId> path_f;
        for (core::NodeId cur = meeting_node; cur != core::kInvalidNode; cur = parent_f[cur]) {
            path_f.push_back(cur);
            if (cur == source) break;
        }
        std::reverse(path_f.begin(), path_f.end());

        // Backward portion: meeting_node -> target
        std::vector<core::NodeId> path_b;
        for (core::NodeId cur = parent_b[meeting_node]; cur != core::kInvalidNode; cur = parent_b[cur]) {
            path_b.push_back(cur);
            if (cur == target) break;
        }

        path_f.insert(path_f.end(), path_b.begin(), path_b.end());
        res.path = std::move(path_f);
    }

    return res;
}

} // namespace graphflow::algorithms
