#include "graphflow/algorithms/max_flow.hpp"
#include "graphflow/graph/graph.hpp"
#include <iostream>
#include <cassert>

using namespace std;
using namespace graphflow::core;
using namespace graphflow::graph;
using namespace graphflow::algorithms;

void test_classic_max_flow() {
    DynamicGraph g(6, true);
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

    auto dinic_res = MaxFlow::dinic(g, 0, 5);
    auto hlpp_res = MaxFlow::push_relabel_hlpp(g, 0, 5);

    assert(dinic_res.max_flow == 23);
    assert(hlpp_res.max_flow == 23);

    assert(!dinic_res.min_cut_source_side.empty());
    bool has_sink = false;
    for (auto u : dinic_res.min_cut_source_side) {
        if (u == 5) has_sink = true;
    }
    assert(!has_sink);

    cout << "test_classic_max_flow passed\n";
}

void test_min_cost_flow() {
    DynamicGraph g(4, true);
    g.add_flow_edge(0, 1, 3, 1);
    g.add_flow_edge(0, 2, 2, 2);
    g.add_flow_edge(1, 3, 2, 2);
    g.add_flow_edge(2, 3, 3, 1);
    g.add_flow_edge(1, 2, 1, 1);

    auto mcmf_res = MaxFlow::min_cost_max_flow(g, 0, 3);
    assert(mcmf_res.max_flow == 5);
    assert(mcmf_res.min_cost == 15);

    cout << "test_min_cost_flow passed\n";
}

int main() {
    test_classic_max_flow();
    test_min_cost_flow();
    return 0;
}
