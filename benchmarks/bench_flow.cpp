#include "graphflow/algorithms/max_flow.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <iomanip>

using namespace graphflow;

int main() {
    std::cout << "=================================================================\n";
    std::cout << " GraphFlow Network Flow Maximization: Dinic vs Push-Relabel (HLPP)\n";
    std::cout << "=================================================================\n";

    size_t num_layers = 20;
    size_t nodes_per_layer = 50;

    std::cout << "Generating layered flow network (" << num_layers << " layers, "
              << nodes_per_layer << " nodes/layer, ~" << (2 + num_layers * nodes_per_layer) << " total nodes)...\n\n";

    auto g = graph::GraphGenerator::generate_flow_network(num_layers, nodes_per_layer, 50, 42);
    core::NodeId source = 0;
    core::NodeId sink = static_cast<core::NodeId>(g.num_nodes() - 1);

    // 1. Dinic's Algorithm
    std::cout << "[1/2] Benchmarking Dinic's Algorithm (Level Graph + Blocking Flow)...\n";
    auto dinic_res = algorithms::MaxFlow::dinic(g, source, sink);
    std::cout << "  Dinic Max Flow: " << dinic_res.max_flow
              << " | Time: " << std::fixed << std::setprecision(2) << dinic_res.execution_time_ms << " ms\n\n";

    // 2. Highest-Label Preflow-Push (Push-Relabel) with Gap Heuristic
    std::cout << "[2/2] Benchmarking Push-Relabel HLPP (Gap Heuristic + Global Relabel)...\n";
    auto hlpp_res = algorithms::MaxFlow::push_relabel_hlpp(g, source, sink);
    std::cout << "  HLPP Max Flow:  " << hlpp_res.max_flow
              << " | Time: " << std::fixed << std::setprecision(2) << hlpp_res.execution_time_ms << " ms\n\n";

    std::cout << "-----------------------------------------------------------------\n";
    if (dinic_res.max_flow == hlpp_res.max_flow) {
        std::cout << " Verified: Both algorithms calculated identical Max Flow (" << dinic_res.max_flow << ")!\n";
    }
    std::cout << "=================================================================\n";

    return 0;
}
