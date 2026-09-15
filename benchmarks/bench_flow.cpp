#include "graphflow/algorithms/max_flow.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace graphflow::core;
using namespace graphflow::graph;
using namespace graphflow::algorithms;

int main() {
    cout << "Network Flow Benchmark: Dinic vs Push-Relabel (HLPP)\n";
    cout << "-----------------------------------------------------\n";

    size_t num_layers = 20;
    size_t nodes_per_layer = 50;
    size_t total_nodes = 2 + num_layers * nodes_per_layer;

    cout << "Generating layered network (" << total_nodes << " nodes, " << num_layers << " layers)...\n";
    auto g = GraphGenerator::generate_flow_network(num_layers, nodes_per_layer, 50, 42);
    NodeId source = 0;
    NodeId sink = static_cast<NodeId>(g.num_nodes() - 1);

    auto dinic_res = MaxFlow::dinic(g, source, sink);
    cout << "  Dinic:       Max Flow = " << dinic_res.max_flow
         << " | Time = " << fixed << setprecision(2) << dinic_res.execution_time_ms << " ms\n";

    auto hlpp_res = MaxFlow::push_relabel_hlpp(g, source, sink);
    cout << "  HLPP:        Max Flow = " << hlpp_res.max_flow
         << " | Time = " << fixed << setprecision(2) << hlpp_res.execution_time_ms << " ms\n";

    if (dinic_res.max_flow == hlpp_res.max_flow) {
        cout << "  Check: Output match (" << dinic_res.max_flow << " units)\n";
    }

    return 0;
}
