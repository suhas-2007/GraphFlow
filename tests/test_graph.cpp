#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <cassert>

using namespace std;
using namespace graphflow;

void test_dynamic_graph_mutations() {
    graph::DynamicGraph g(5, true);
    g.add_edge(0, 1, 10.0);
    g.add_edge(0, 2, 20.0);
    g.add_edge(1, 3, 30.0);
    g.add_edge(2, 3, 40.0);

    assert(g.num_nodes() == 5);
    assert(g.num_edges() == 4);
    assert(g.has_edge(0, 1));
    assert(g.has_edge(0, 2));
    assert(!g.has_edge(1, 0));

    assert(g.get_edge_weight(0, 1).value() == 10.0);

    g.update_edge_weight(0, 1, 15.5);
    assert(g.get_edge_weight(0, 1).value() == 15.5);

    bool removed = g.remove_edge(0, 1);
    assert(removed);
    assert(!g.has_edge(0, 1));
    assert(g.num_edges() == 3);

    cout << "test_dynamic_graph_mutations passed\n";
}

void test_csr_conversion() {
    graph::DynamicGraph g(4, true);
    g.add_edge(0, 1, 5.0);
    g.add_edge(0, 2, 6.0);
    g.add_edge(1, 2, 7.0);
    g.add_edge(2, 3, 8.0);

    auto csr = g.to_csr();
    assert(csr.num_nodes == 4);
    assert(csr.num_edges == 4);
    assert(csr.degree(0) == 2);
    assert(csr.degree(1) == 1);
    assert(csr.degree(2) == 1);
    assert(csr.degree(3) == 0);

    auto n0 = csr.neighbors(0);
    assert(n0.size() == 2);
    assert(n0[0] == 1 && n0[1] == 2);

    cout << "test_csr_conversion passed\n";
}

void test_bitset_adjacency() {
    graph::BitsetAdjacencyList bg(10);
    bg.add_edge(0, 1, 1.0f);
    bg.add_edge(0, 2, 2.0f);
    bg.add_edge(3, 1, 3.0f);
    bg.add_edge(3, 2, 4.0f);
    bg.add_edge(3, 5, 5.0f);

    assert(bg.has_edge(0, 1));
    assert(bg.has_edge(0, 2));
    assert(!bg.has_edge(0, 3));
    assert(!bg.has_edge(1, 0));

    // Common neighbors between 0 and 3 are 1 and 2
    size_t cn = bg.common_neighbors(0, 3);
    assert(cn == 2);

    cout << "test_bitset_adjacency passed\n";
}

int main() {
    test_dynamic_graph_mutations();
    test_csr_conversion();
    test_bitset_adjacency();
    return 0;
}
