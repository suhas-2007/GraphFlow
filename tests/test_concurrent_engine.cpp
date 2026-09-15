#include "graphflow/concurrency/concurrent_engine.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace std;
using namespace graphflow::core;
using namespace graphflow::graph;
using namespace graphflow::concurrency;

int main() {
    auto g = GraphGenerator::generate_random_graph(1000, 5000, 1.0, 10.0, true, 42);

    ConcurrentEngine engine(4);
    assert(engine.num_workers() == 4);

    vector<QueryPair> queries;
    for (NodeId i = 0; i < 50; ++i) {
        queries.push_back({.source = i, .target = static_cast<NodeId>(1000 - 1 - i)});
    }

    auto results = engine.execute_batch_queries(g, queries);
    assert(results.size() == 50);

    for (const auto& res : results) {
        if (res.distance < kInfinityWeight) {
            assert(!res.path.empty());
            assert(res.nodes_visited > 0);
        }
    }

    cout << "test_concurrent_engine passed\n";
    return 0;
}
