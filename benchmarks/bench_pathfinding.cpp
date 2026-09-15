#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>

using namespace std;
using namespace std::chrono;
using namespace graphflow::core;
using namespace graphflow::graph;
using namespace graphflow::algorithms;

int main(int argc, char* argv[]) {
    size_t num_nodes = 500000;
    size_t avg_degree = 16;

    if (argc >= 2) {
        num_nodes = stoull(argv[1]);
    }
    if (argc >= 3) {
        avg_degree = stoull(argv[2]);
    }

    cout << "Pathfinding Benchmark: 500,000+ Node Topologies\n";
    cout << "Topology: " << num_nodes << " nodes, ~" << (num_nodes * avg_degree) << " edges\n";
    cout << "---------------------------------------------------------\n";

    cout << "Generating test graph...\n";
    auto t0 = steady_clock::now();
    auto dyn_graph = GraphGenerator::generate_random_graph(num_nodes, num_nodes * avg_degree, 1.0, 25.0, true, 42);
    auto t1 = steady_clock::now();
    cout << "  DynamicGraph ready (" << duration<double>(t1 - t0).count() << " s)\n";

    BitsetAdjacencyList bitset_graph(num_nodes);
    for (size_t u = 0; u < num_nodes; ++u) {
        for (const auto& e : dyn_graph.out_edges(static_cast<NodeId>(u))) {
            bitset_graph.add_edge(static_cast<NodeId>(u), e.target, static_cast<float>(e.weight));
        }
    }
    auto t2 = steady_clock::now();
    cout << "  BitsetAdjacencyList ready (" << duration<double>(t2 - t1).count() << " s)\n\n";

    vector<pair<NodeId, NodeId>> query_pairs = {
        {0, static_cast<NodeId>(num_nodes / 4)},
        {0, static_cast<NodeId>(num_nodes / 2)},
        {static_cast<NodeId>(num_nodes / 8), static_cast<NodeId>(num_nodes * 3 / 4)},
        {static_cast<NodeId>(num_nodes / 10), static_cast<NodeId>(num_nodes / 3)},
    };

    double total_std_ms = 0.0;
    double total_4ary_ms = 0.0;
    double total_bitset_ms = 0.0;
    double total_bidi_ms = 0.0;

    for (size_t i = 0; i < query_pairs.size(); ++i) {
        auto [src, dst] = query_pairs[i];
        cout << "Query " << (i + 1) << "/" << query_pairs.size()
             << " (" << src << " -> " << dst << "):\n";

        // Baseline: std::priority_queue
        auto res_std = ShortestPath::dijkstra_std(dyn_graph, src, dst);
        total_std_ms += res_std.execution_time_ms;
        cout << "  std::priority_queue:   " << fixed << setprecision(2)
             << res_std.execution_time_ms << " ms | Visited: " << res_std.nodes_visited << "\n";

        // 4-ary Indexed Heap
        auto res_4ary = ShortestPath::dijkstra_indexed_4ary(dyn_graph, src, dst);
        total_4ary_ms += res_4ary.execution_time_ms;
        cout << "  4-ary Indexed Heap:    " << fixed << setprecision(2)
             << res_4ary.execution_time_ms << " ms | Visited: " << res_4ary.nodes_visited << "\n";

        // Bitset Adjacency + 4-ary Heap
        auto res_bitset = ShortestPath::dijkstra_bitset(bitset_graph, src, dst);
        total_bitset_ms += res_bitset.execution_time_ms;
        cout << "  Bitset + 4-ary Heap:   " << fixed << setprecision(2)
             << res_bitset.execution_time_ms << " ms | Visited: " << res_bitset.nodes_visited << "\n";

        // Bidirectional Dijkstra
        auto res_bidi = ShortestPath::bidirectional_dijkstra(dyn_graph, src, dst);
        total_bidi_ms += res_bidi.execution_time_ms;
        cout << "  Bidirectional 4-ary:   " << fixed << setprecision(2)
             << res_bidi.execution_time_ms << " ms | Visited: " << res_bidi.nodes_visited << "\n\n";
    }

    double avg_std = total_std_ms / query_pairs.size();
    double avg_4ary = total_4ary_ms / query_pairs.size();
    double avg_bitset = total_bitset_ms / query_pairs.size();
    double avg_bidi = total_bidi_ms / query_pairs.size();

    double speedup_bitset = (avg_std - avg_bitset) / avg_std * 100.0;
    double speedup_bidi = (avg_std - avg_bidi) / avg_std * 100.0;

    cout << "Average Execution Times:\n";
    cout << "  std::priority_queue:   " << fixed << setprecision(2) << avg_std << " ms\n";
    cout << "  4-ary Indexed Heap:    " << avg_4ary << " ms\n";
    cout << "  Bitset + 4-ary Heap:   " << avg_bitset << " ms (" << setprecision(1) << speedup_bitset << "% faster)\n";
    cout << "  Bidirectional 4-ary:   " << avg_bidi << " ms (" << setprecision(1) << speedup_bidi << "% faster)\n";

    return 0;
}
