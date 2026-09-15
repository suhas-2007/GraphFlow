#include "graphflow/algorithms/dynamic_programming.hpp"
#include "graphflow/graph/graph.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace graphflow;

void test_critical_path_method() {
    // Project scheduling DAG:
    // 0 -> 1 (wt 3)
    // 0 -> 2 (wt 2)
    // 1 -> 3 (wt 4)
    // 2 -> 3 (wt 1)
    // 3 -> 4 (wt 2)
    graph::DynamicGraph dag(5, true);
    dag.add_edge(0, 1, 3.0);
    dag.add_edge(0, 2, 2.0);
    dag.add_edge(1, 3, 4.0);
    dag.add_edge(2, 3, 1.0);
    dag.add_edge(3, 4, 2.0);

    auto res_opt = algorithms::DynamicProgramming::critical_path_method(dag);
    assert(res_opt.has_value());
    const auto& res = res_opt.value();

    // Critical path: 0 -> 1 -> 3 -> 4, length = 3 + 4 + 2 = 9.0
    assert(std::abs(res.critical_path_length - 9.0) < 1e-6);
    assert(res.critical_path.size() == 4);
    assert(res.critical_path[0] == 0);
    assert(res.critical_path[1] == 1);
    assert(res.critical_path[2] == 3);
    assert(res.critical_path[3] == 4);

    // Slack of nodes on critical path should be 0.0
    assert(std::abs(res.slack[0]) < 1e-6);
    assert(std::abs(res.slack[1]) < 1e-6);
    assert(std::abs(res.slack[3]) < 1e-6);
    assert(std::abs(res.slack[4]) < 1e-6);
    // Node 2 has slack = 4.0 (2 + 1 vs 3 + 4)
    assert(res.slack[2] > 0.0);

    std::cout << "[PASS] test_critical_path_method\n";
}

void test_path_counting() {
    graph::DynamicGraph dag(4, true);
    dag.add_edge(0, 1, 1.0);
    dag.add_edge(0, 2, 1.0);
    dag.add_edge(1, 3, 1.0);
    dag.add_edge(2, 3, 1.0);
    dag.add_edge(0, 3, 1.0);

    // Paths: 0->3, 0->1->3, 0->2->3 => 3 paths
    uint64_t paths = algorithms::DynamicProgramming::count_paths_dag(dag, 0, 3);
    assert(paths == 3);

    std::cout << "[PASS] test_path_counting\n";
}

void test_tsp_bitmask() {
    // Complete 4-node graph
    graph::DynamicGraph g(4, true);
    // 0-1-2-3-0 cycle of cost 10 each = 40
    // diagonals higher cost
    g.add_edge(0, 1, 10.0); g.add_edge(1, 0, 10.0);
    g.add_edge(1, 2, 10.0); g.add_edge(2, 1, 10.0);
    g.add_edge(2, 3, 10.0); g.add_edge(3, 2, 10.0);
    g.add_edge(3, 0, 10.0); g.add_edge(0, 3, 10.0);
    g.add_edge(0, 2, 50.0); g.add_edge(2, 0, 50.0);
    g.add_edge(1, 3, 50.0); g.add_edge(3, 1, 50.0);

    auto tsp_res = algorithms::DynamicProgramming::solve_tsp_bitmask(g, 0);
    assert(std::abs(tsp_res.min_cost - 40.0) < 1e-6);
    assert(tsp_res.tour.size() == 5); // Returns to start
    assert(tsp_res.tour.front() == 0 && tsp_res.tour.back() == 0);

    std::cout << "[PASS] test_tsp_bitmask (Min tour = 40.0)\n";
}

int main() {
    std::cout << "--- Running Dynamic Programming Tests ---\n";
    test_critical_path_method();
    test_path_counting();
    test_tsp_bitmask();
    std::cout << "All Dynamic Programming tests PASSED!\n";
    return 0;
}
