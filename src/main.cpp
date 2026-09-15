#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "graphflow/core/types.hpp"
#include "graphflow/core/arena_allocator.hpp"
#include "graphflow/core/dary_heap.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/algorithms/max_flow.hpp"
#include "graphflow/algorithms/dynamic_programming.hpp"
#include "graphflow/algorithms/traversal.hpp"
#include "graphflow/concurrency/concurrent_engine.hpp"

using namespace graphflow;

void print_banner() {
    std::cout << R"(
  ██████╗ ██████╗  █████╗ ██████╗ ██╗  ██╗███████╗██╗      ██████╗ ██╗    ██╗
 ██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██║  ██║██╔════╝██║     ██╔═══██╗██║    ██║
 ██║  ███╗██████╔╝███████║██████╔╝███████║█████╗  ██║     ██║   ██║██║ █╗ ██║
 ██║   ██║██╔══██╗██╔══██║██╔═══╝ ██╔══██║██╔══╝  ██║     ██║   ██║██║███╗██║
 ╚██████╔╝██║  ██║██║  ██║██║     ██║  ██║██║     ███████╗╚██████╔╝╚███╔███╔╝
  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝     ╚══════╝ ╚═════╝  ╚══╝╚══╝ 
    High-Performance Algorithmic Graph Engine [C++20 / DSA / Dynamic Systems]
    Bitset Adjacency | 4-ary Indexed Heap | Arena Allocator | Dinic & HLPP
)" << "\n";
}

void run_pathfinding_demo() {
    std::cout << "\n=======================================================\n";
    std::cout << " [1] Pathfinding Acceleration Demo (Bitset + 4-ary Heap)\n";
    std::cout << "=======================================================\n";

    size_t n = 100000;
    std::cout << "Generating 100,000 node graph...\n";
    auto bitset_g = graph::GraphGenerator::generate_benchmark_bitset_graph(n, 6, 1.0f, 20.0f, 1337);
    auto dyn_g = graph::GraphGenerator::generate_random_graph(n, n * 6, 1.0, 20.0, true, 1337);

    core::NodeId src = 0;
    core::NodeId dst = static_cast<core::NodeId>(n - 1);

    std::cout << "Running Baseline std::priority_queue Dijkstra (0 -> " << dst << ")...\n";
    auto std_res = algorithms::ShortestPath::dijkstra_std(dyn_g, src, dst);
    std::cout << "  Baseline Time: " << std::fixed << std::setprecision(2) << std_res.execution_time_ms
              << " ms | Visited: " << std_res.nodes_visited << " nodes | Dist: " << std_res.distance << "\n";

    std::cout << "Running GraphFlow Bitset + 4-ary Indexed Heap Dijkstra...\n";
    auto gf_res = algorithms::ShortestPath::dijkstra_bitset(bitset_g, src, dst);
    std::cout << "  GraphFlow Time: " << std::fixed << std::setprecision(2) << gf_res.execution_time_ms
              << " ms | Visited: " << gf_res.nodes_visited << " nodes | Dist: " << gf_res.distance << "\n";

    double speedup = (std_res.execution_time_ms - gf_res.execution_time_ms) / std_res.execution_time_ms * 100.0;
    std::cout << ">> Result: " << std::setprecision(1) << speedup << "% latency reduction!\n";
}

void run_flow_demo() {
    std::cout << "\n=======================================================\n";
    std::cout << " [2] Network Flow Maximization (Dinic vs Push-Relabel)\n";
    std::cout << "=======================================================\n";

    auto flow_g = graph::GraphGenerator::generate_flow_network(10, 20, 100, 42);
    core::NodeId s = 0;
    core::NodeId t = static_cast<core::NodeId>(flow_g.num_nodes() - 1);

    std::cout << "Evaluating 200-node layered flow network...\n";

    auto dinic_res = algorithms::MaxFlow::dinic(flow_g, s, t);
    std::cout << "  Dinic Max Flow: " << dinic_res.max_flow
              << " | Time: " << std::fixed << std::setprecision(2) << dinic_res.execution_time_ms << " ms\n";

    auto hlpp_res = algorithms::MaxFlow::push_relabel_hlpp(flow_g, s, t);
    std::cout << "  HLPP Max Flow:  " << hlpp_res.max_flow
              << " | Time: " << std::fixed << std::setprecision(2) << hlpp_res.execution_time_ms << " ms\n";
}

void run_dp_demo() {
    std::cout << "\n=======================================================\n";
    std::cout << " [3] State-Space Dynamic Programming (Critical Path & TSP)\n";
    std::cout << "=======================================================\n";

    // DAG Critical Path
    auto dag = graph::GraphGenerator::generate_dag(15, 0.3, 1.0, 10.0, 42);
    auto cpm_res = algorithms::DynamicProgramming::critical_path_method(dag);
    if (cpm_res) {
        std::cout << "  DAG Critical Path Length: " << cpm_res->critical_path_length << "\n";
        std::cout << "  Bottleneck Path: ";
        for (size_t i = 0; i < cpm_res->critical_path.size(); ++i) {
            std::cout << cpm_res->critical_path[i] << (i + 1 < cpm_res->critical_path.size() ? " -> " : "\n");
        }
    }

    // TSP Bitmask
    graph::DynamicGraph complete_g(6, true);
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 6; ++j) {
            if (i != j) complete_g.add_edge(static_cast<core::NodeId>(i), static_cast<core::NodeId>(j), (i + j) % 5 + 1.0);
        }
    }
    auto tsp = algorithms::DynamicProgramming::solve_tsp_bitmask(complete_g, 0);
    std::cout << "  State-Space Bitmask TSP Min Tour Cost: " << tsp.min_cost << "\n";
}

void run_concurrent_demo() {
    std::cout << "\n=======================================================\n";
    std::cout << " [4] Concurrent Query Engine & Thread-Local Arena Alloc\n";
    std::cout << "=======================================================\n";

    concurrency::ConcurrentEngine engine(4);
    std::cout << "Initialized engine with " << engine.num_workers() << " worker threads.\n";

    auto g = graph::GraphGenerator::generate_random_graph(5000, 25000, 1.0, 10.0, true, 42);
    std::vector<concurrency::QueryPair> queries;
    for (size_t i = 0; i < 100; ++i) {
        queries.push_back({.source = static_cast<core::NodeId>(i), .target = static_cast<core::NodeId>(4999 - i)});
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    auto results = engine.execute_batch_queries(g, queries);
    auto t1 = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "  Executed " << results.size() << " concurrent queries in "
              << std::fixed << std::setprecision(2) << ms << " ms ("
              << std::setprecision(0) << (results.size() / (ms / 1000.0)) << " QPS)\n";
    std::cout << "  Allocation Overhead: O(1) monotonic bump pointers with zero lock contention.\n";
}

int main(int argc, char* argv[]) {
    print_banner();
    run_pathfinding_demo();
    run_flow_demo();
    run_dp_demo();
    run_concurrent_demo();

    std::cout << "\n=======================================================\n";
    std::cout << " GraphFlow Engine Initialized Successfully!\n";
    std::cout << "=======================================================\n";
    return 0;
}
