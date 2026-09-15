#include "graphflow/algorithms/max_flow.hpp"
#include "graphflow/graph/graph.hpp"
#include <iostream>
#include <cassert>

using namespace graphflow;

void test_classic_max_flow() {
    // 6-node classic network flow graph
    // Source: 0, Sink: 5
    graph::DynamicGraph g(6, true);
    g.add_flow_edge(0, 1, 16);
    g.add_flow_edge(0, 2, 13);
    g.add_flow_edge(1, 2, 10);
    g.add_flow_edge(1, 3, 12);
    g.add_flow_edge(2, 1, 4);
    g.add_flow_edge(2, 4, 14);
    g.add_flow_edge(3, 2, 9);
    g.add_flow_edge(3, 5, 20);
    g.add_flow_edge(4, 3, 7);
    g.add_flow_edge(4, 5, 4);

    auto dinic_res = algorithms::MaxFlow::dinic(g, 0, 5);
    auto hlpp_res = algorithms::MaxFlow::push_relabel_hlpp(g, 0, 5);

    // Expected maximum flow is 23
    assert(dinic_res.max_flow == 23);
    assert(hlpp_res.max_flow == 23);

    // Verify min-cut size from source side: source side has at least node 0, does not have node 5
    assert(!dinic_res.min_cut_source_side.empty());
    bool has_sink = false;
    for (auto u : dinic_res.min_cut_source_side) {
        if (u == 5) has_sink = true;
    }
    assert(!has_sink);

    std::cout << "[PASS] test_classic_max_flow (Max flow = 23)\n";
}

void test_min_cost_flow() {
    // 4-node flow network with costs
    // 0 -> 1: cap 3, cost 1
    // 0 -> 2: cap 2, cost 2
    // 1 -> 3: cap 2, cost 2
    // 2 -> 3: cap 3, cost 1
    // 1 -> 2: cap 1, cost 1
    graph::DynamicGraph g(4, true);
    g.add_flow_edge(0, 1, 3, 1);
    g.add_flow_edge(0, 2, 2, 2);
    g.add_flow_edge(1, 3, 2, 2);
    g.add_flow_edge(2, 3, 3, 1);
    g.add_flow_edge(1, 2, 1, 1);

    auto mcmf_res = algorithms::MaxFlow::min_cost_max_flow(g, 0, 3);
    // Max flow is 5, minimum cost = 2*(1+2) + 2*(2+1) + 1*(1+1+1) = 6 + 6 + 3 = 15
    assert(mcmf_res.max_flow == 5);
    assert(mcmf_res.min_cost == 15);

    std::cout << "[PASS] test_min_cost_flow (Flow = 5, Min Cost = 15)\n";
}

int main() {
    std::cout << "--- Running Max Flow & Min Cost Tests ---\n";
    test_classic_max_flow();
    test_min_cost_flow();
    std::cout << "All Max Flow tests PASSED!\n";
    return 0;
}
