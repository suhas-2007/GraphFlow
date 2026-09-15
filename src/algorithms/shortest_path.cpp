#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/core/dary_heap.hpp"
#include <queue>
#include <chrono>
#include <algorithm>
#include <functional>

using namespace std;
using namespace std::chrono;
using namespace graphflow::core;
using namespace graphflow::graph;

namespace graphflow::algorithms {

namespace {

vector<NodeId> reconstruct_path(
    const vector<NodeId>& parent,
    NodeId source,
    NodeId target
) {
    if (target == kInvalidNode || (parent[target] == kInvalidNode && target != source)) {
        return {};
    }

    vector<NodeId> path;
    for (NodeId cur = target; cur != kInvalidNode; cur = parent[cur]) {
        path.push_back(cur);
        if (cur == source) break;
    }
    reverse(path.begin(), path.end());
    return path;
}

} // namespace

PathResult ShortestPath::dijkstra_std(
    const DynamicGraph& g,
    NodeId source,
    NodeId target
) {
    auto start_time = steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = kInfinityWeight};
    }

    vector<EdgeWeight> dist(n, kInfinityWeight);
    vector<NodeId> parent(n, kInvalidNode);
    vector<bool> visited(n, false);

    using QueueElement = pair<EdgeWeight, NodeId>;
    priority_queue<QueueElement, vector<QueueElement>, greater<QueueElement>> pq;

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
            NodeId v = edge.target;
            EdgeWeight new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                pq.push({new_dist, v});
            }
        }
    }

    auto end_time = steady_clock::now();
    double ms = duration<double, milli>(end_time - start_time).count();

    PathResult res;
    res.distance = (target != kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

PathResult ShortestPath::dijkstra_indexed_4ary(
    const DynamicGraph& g,
    NodeId source,
    NodeId target
) {
    auto start_time = steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = kInfinityWeight};
    }

    vector<EdgeWeight> dist(n, kInfinityWeight);
    vector<NodeId> parent(n, kInvalidNode);

    IndexedDaryHeap<NodeId, EdgeWeight, 4> heap(n);

    dist[source] = 0.0;
    heap.push(source, 0.0);

    uint64_t nodes_visited = 0;

    while (!heap.empty()) {
        auto min_entry = heap.pop();
        NodeId u = min_entry.key;
        EdgeWeight d = min_entry.priority;

        nodes_visited++;
        if (u == target) break;

        for (const auto& edge : g.out_edges(u)) {
            NodeId v = edge.target;
            EdgeWeight new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                heap.push_or_decrease_key(v, new_dist);
            }
        }
    }

    auto end_time = steady_clock::now();
    double ms = duration<double, milli>(end_time - start_time).count();

    PathResult res;
    res.distance = (target != kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

PathResult ShortestPath::dijkstra_bitset(
    const BitsetAdjacencyList& g,
    NodeId source,
    NodeId target
) {
    auto start_time = steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n) {
        return {.distance = kInfinityWeight};
    }

    vector<float> dist(n, static_cast<float>(kInfinityWeight));
    vector<NodeId> parent(n, kInvalidNode);

    IndexedDaryHeap<NodeId, float, 4> heap(n);

    dist[source] = 0.0f;
    heap.push(source, 0.0f);

    uint64_t nodes_visited = 0;

    while (!heap.empty()) {
        auto min_entry = heap.pop();
        NodeId u = min_entry.key;
        float d = min_entry.priority;

        nodes_visited++;
        if (u == target) break;

        for (const auto& edge : g.neighbors(u)) {
            NodeId v = edge.target;
            float new_dist = d + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                parent[v] = u;
                heap.push_or_decrease_key(v, new_dist);
            }
        }
    }

    auto end_time = steady_clock::now();
    double ms = duration<double, milli>(end_time - start_time).count();

    PathResult res;
    res.distance = (target != kInvalidNode && target < n) ? dist[target] : 0.0;
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

PathResult ShortestPath::a_star(
    const DynamicGraph& g,
    NodeId source,
    NodeId target,
    function<EdgeWeight(NodeId, NodeId)> heuristic
) {
    auto start_time = steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || target >= n) {
        return {.distance = kInfinityWeight};
    }

    vector<EdgeWeight> g_score(n, kInfinityWeight);
    vector<NodeId> parent(n, kInvalidNode);

    IndexedDaryHeap<NodeId, EdgeWeight, 4> open_set(n);

    g_score[source] = 0.0;
    open_set.push(source, heuristic(source, target));

    uint64_t nodes_visited = 0;

    while (!open_set.empty()) {
        auto min_entry = open_set.pop();
        NodeId u = min_entry.key;
        nodes_visited++;

        if (u == target) break;

        for (const auto& edge : g.out_edges(u)) {
            NodeId v = edge.target;
            EdgeWeight tentative_g = g_score[u] + edge.weight;

            if (tentative_g < g_score[v]) {
                parent[v] = u;
                g_score[v] = tentative_g;
                EdgeWeight f_score = tentative_g + heuristic(v, target);
                open_set.push_or_decrease_key(v, f_score);
            }
        }
    }

    auto end_time = steady_clock::now();
    double ms = duration<double, milli>(end_time - start_time).count();

    PathResult res;
    res.distance = g_score[target];
    res.path = reconstruct_path(parent, source, target);
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;
    return res;
}

PathResult ShortestPath::bidirectional_dijkstra(
    const DynamicGraph& g,
    NodeId source,
    NodeId target
) {
    auto start_time = steady_clock::now();
    const size_t n = g.num_nodes();

    if (source >= n || target >= n) {
        return {.distance = kInfinityWeight};
    }
    if (source == target) {
        return {.distance = 0.0, .path = {source}, .nodes_visited = 1, .execution_time_ms = 0.0};
    }

    vector<EdgeWeight> dist_f(n, kInfinityWeight);
    vector<EdgeWeight> dist_b(n, kInfinityWeight);
    vector<NodeId> parent_f(n, kInvalidNode);
    vector<NodeId> parent_b(n, kInvalidNode);
    vector<bool> visited_f(n, false);
    vector<bool> visited_b(n, false);

    IndexedDaryHeap<NodeId, EdgeWeight, 4> heap_f(n);
    IndexedDaryHeap<NodeId, EdgeWeight, 4> heap_b(n);

    vector<vector<Edge>> rev_adj(n);
    for (size_t u = 0; u < n; ++u) {
        for (const auto& e : g.out_edges(static_cast<NodeId>(u))) {
            rev_adj[e.target].push_back({static_cast<NodeId>(u), e.weight});
        }
    }

    dist_f[source] = 0.0;
    heap_f.push(source, 0.0);

    dist_b[target] = 0.0;
    heap_b.push(target, 0.0);

    EdgeWeight best_dist = kInfinityWeight;
    NodeId meeting_node = kInvalidNode;
    uint64_t nodes_visited = 0;

    while (!heap_f.empty() && !heap_b.empty()) {
        if (!heap_f.empty()) {
            auto [u_f, d_f] = heap_f.pop();
            visited_f[u_f] = true;
            nodes_visited++;

            if (d_f + (heap_b.empty() ? 0.0 : heap_b.top().priority) >= best_dist) {
                break;
            }

            for (const auto& edge : g.out_edges(u_f)) {
                NodeId v = edge.target;
                EdgeWeight nd = d_f + edge.weight;
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

        if (!heap_b.empty()) {
            auto [u_b, d_b] = heap_b.pop();
            visited_b[u_b] = true;
            nodes_visited++;

            for (const auto& edge : rev_adj[u_b]) {
                NodeId v = edge.target;
                EdgeWeight nd = d_b + edge.weight;
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

    auto end_time = steady_clock::now();
    double ms = duration<double, milli>(end_time - start_time).count();

    PathResult res;
    res.distance = best_dist;
    res.nodes_visited = nodes_visited;
    res.execution_time_ms = ms;

    if (meeting_node != kInvalidNode) {
        vector<NodeId> path_f;
        for (NodeId cur = meeting_node; cur != kInvalidNode; cur = parent_f[cur]) {
            path_f.push_back(cur);
            if (cur == source) break;
        }
        reverse(path_f.begin(), path_f.end());

        vector<NodeId> path_b;
        for (NodeId cur = parent_b[meeting_node]; cur != kInvalidNode; cur = parent_b[cur]) {
            path_b.push_back(cur);
            if (cur == target) break;
        }

        path_f.insert(path_f.end(), path_b.begin(), path_b.end());
        res.path = move(path_f);
    }

    return res;
}

} // namespace graphflow::algorithms
