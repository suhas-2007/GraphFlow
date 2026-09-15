#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>

using namespace graphflow;

int main(int argc, char* argv[]) {
    size_t num_nodes = 500000;
    size_t avg_degree = 16;

    if (argc >= 2) {
        num_nodes = std::stoull(argv[1]);
    }
    if (argc >= 3) {
        avg_degree = std::stoull(argv[2]);
    }

    std::cout << "=================================================================\n";
    std::cout << " GraphFlow Pathfinding Acceleration Benchmark (500,000+ Nodes)\n";
    std::cout << "=================================================================\n";
    std::cout << "Graph Topology: " << num_nodes << " nodes, ~" << (num_nodes * avg_degree) << " edges\n";

    std::cout << "\n[1/3] Generating shared canonical graph...\n";
    auto t0 = std::chrono::steady_clock::now();
    auto dyn_graph = graph::GraphGenerator::generate_random_graph(num_nodes, num_nodes * avg_degree, 1.0, 25.0, true, 42);
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "  Generated Dynamic Graph in "
              << std::chrono::duration<double>(t1 - t0).count() << " s\n";

    // Populate BitsetAdjacencyList from the exact same edges
    graph::BitsetAdjacencyList bitset_graph(num_nodes);
    for (size_t u = 0; u < num_nodes; ++u) {
        for (const auto& e : dyn_graph.out_edges(static_cast<core::NodeId>(u))) {
            bitset_graph.add_edge(static_cast<core::NodeId>(u), e.target, static_cast<float>(e.weight));
        }
    }
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "  Constructed Bitset Adjacency Graph in "
              << std::chrono::duration<double>(t2 - t1).count() << " s\n";

    // Benchmark across representative query pairs
    std::vector<std::pair<core::NodeId, core::NodeId>> query_pairs = {
        {0, static_cast<core::NodeId>(num_nodes / 4)},
        {0, static_cast<core::NodeId>(num_nodes / 2)},
        {static_cast<core::NodeId>(num_nodes / 8), static_cast<core::NodeId>(num_nodes * 3 / 4)},
        {static_cast<core::NodeId>(num_nodes / 10), static_cast<core::NodeId>(num_nodes / 3)},
    };

    std::cout << "\n[2/3] Benchmarking " << query_pairs.size() << " queries on 500,000+ nodes...\n";

    double total_std_ms = 0.0;
    double total_4ary_ms = 0.0;
    double total_bitset_ms = 0.0;
    double total_bidi_ms = 0.0;

    for (size_t i = 0; i < query_pairs.size(); ++i) {
        auto [src, dst] = query_pairs[i];
        std::cout << "\n  --- Query " << (i + 1) << "/" << query_pairs.size()
                  << " (Source: " << src << " -> Target: " << dst << ") ---\n";

        // Baseline: std::priority_queue
        auto res_std = algorithms::ShortestPath::dijkstra_std(dyn_graph, src, dst);
        total_std_ms += res_std.execution_time_ms;
        std::cout << "    Baseline (std::priority_queue): " << std::fixed << std::setprecision(2)
                  << res_std.execution_time_ms << " ms | Visited: " << res_std.nodes_visited
                  << " | Dist: " << res_std.distance << "\n";

        // GraphFlow: 4-ary Indexed Heap
        auto res_4ary = algorithms::ShortestPath::dijkstra_indexed_4ary(dyn_graph, src, dst);
        total_4ary_ms += res_4ary.execution_time_ms;
        std::cout << "    GraphFlow (4-ary Indexed Heap): " << std::fixed << std::setprecision(2)
                  << res_4ary.execution_time_ms << " ms | Visited: " << res_4ary.nodes_visited
                  << " | Dist: " << res_4ary.distance << "\n";

        // GraphFlow: Bitset Adjacency + 4-ary Heap (Forward)
        auto res_bitset = algorithms::ShortestPath::dijkstra_bitset(bitset_graph, src, dst);
        total_bitset_ms += res_bitset.execution_time_ms;
        std::cout << "    GraphFlow (Bitset + 4-ary Heap):" << std::fixed << std::setprecision(2)
                  << res_bitset.execution_time_ms << " ms | Visited: " << res_bitset.nodes_visited
                  << " | Dist: " << res_bitset.distance << "\n";

        // GraphFlow: Bidirectional 4-ary Search
        auto res_bidi = algorithms::ShortestPath::bidirectional_dijkstra(dyn_graph, src, dst);
        total_bidi_ms += res_bidi.execution_time_ms;
        std::cout << "    GraphFlow (Bidirectional 4-ary):" << std::fixed << std::setprecision(2)
                  << res_bidi.execution_time_ms << " ms | Visited: " << res_bidi.nodes_visited
                  << " | Dist: " << res_bidi.distance << "\n";
    }

    std::cout << "\n[3/3] Overall Aggregate Performance:\n";
    double avg_std = total_std_ms / query_pairs.size();
    double avg_4ary = total_4ary_ms / query_pairs.size();
    double avg_bitset = total_bitset_ms / query_pairs.size();
    double avg_bidi = total_bidi_ms / query_pairs.size();

    double speedup_bitset_pct = (avg_std - avg_bitset) / avg_std * 100.0;
    double speedup_bidi_pct = (avg_std - avg_bidi) / avg_std * 100.0;
    double speedup_bidi_factor = avg_std / avg_bidi;

    std::cout << "-----------------------------------------------------------------\n";
    std::cout << " Baseline (std::priority_queue):          " << std::fixed << std::setprecision(2) << avg_std << " ms\n";
    std::cout << " GraphFlow 4-ary Indexed Heap:            " << avg_4ary << " ms\n";
    std::cout << " GraphFlow (Bitset + 4-ary Heap):         " << avg_bitset << " ms (" << std::setprecision(1) << speedup_bitset_pct << "% faster)\n";
    std::cout << " GraphFlow (Bidirectional 4-ary Engine):  " << avg_bidi << " ms (" << std::setprecision(1) << speedup_bidi_pct << "% faster)\n";
    std::cout << " Max Acceleration Factor:                 " << std::setprecision(2) << speedup_bidi_factor << "x faster (" << std::setprecision(1) << speedup_bidi_pct << "% latency reduction)\n";
    std::cout << "=================================================================\n";

    return 0;
}
