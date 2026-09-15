#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/graph_generator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace graphflow;

void test_dijkstra_equivalency() {
    // 5-node test graph
    graph::DynamicGraph g(5, true);
    g.add_edge(0, 1, 4.0);
    g.add_edge(0, 2, 2.0);
    g.add_edge(2, 1, 1.0); // 0 -> 2 -> 1 dist = 3.0 (< 4.0)
    g.add_edge(1, 3, 5.0);
    g.add_edge(2, 3, 8.0);
    g.add_edge(3, 4, 2.0);
    g.add_edge(2, 4, 10.0);

    auto res_std = algorithms::ShortestPath::dijkstra_std(g, 0, 4);
    auto res_4ary = algorithms::ShortestPath::dijkstra_indexed_4ary(g, 0, 4);
    auto res_bidi = algorithms::ShortestPath::bidirectional_dijkstra(g, 0, 4);

    // Shortest path: 0 -> 2 -> 1 -> 3 -> 4, distance = 2 + 1 + 5 + 2 = 10.0
    assert(std::abs(res_std.distance - 10.0) < 1e-6);
    assert(std::abs(res_4ary.distance - 10.0) < 1e-6);
    assert(std::abs(res_bidi.distance - 10.0) < 1e-6);

    assert(res_4ary.path.size() == 5);
    assert(res_4ary.path[0] == 0);
    assert(res_4ary.path[1] == 2);
    assert(res_4ary.path[2] == 1);
    assert(res_4ary.path[3] == 3);
    assert(res_4ary.path[4] == 4);

    std::cout << "[PASS] test_dijkstra_equivalency\n";
}

void test_a_star_grid() {
    // 10x10 Grid
    size_t width = 10;
    size_t height = 10;
    auto grid = graph::GraphGenerator::generate_grid_graph(width, height, 1.0, 1.0, 42);

    core::NodeId start = 0;
    core::NodeId target = static_cast<core::NodeId>(width * height - 1);

    auto manhattan_heuristic = [width](core::NodeId u, core::NodeId v) -> core::EdgeWeight {
        int ux = u % width;
        int uy = u / width;
        int vx = v % width;
        int vy = v / width;
        return std::abs(ux - vx) + std::abs(uy - vy);
    };

    auto dijkstra_res = algorithms::ShortestPath::dijkstra_indexed_4ary(grid, start, target);
    auto astar_res = algorithms::ShortestPath::a_star(grid, start, target, manhattan_heuristic);

    assert(std::abs(dijkstra_res.distance - astar_res.distance) < 1e-6);
    // A* should explore fewer or equal nodes than full Dijkstra
    assert(astar_res.nodes_visited <= dijkstra_res.nodes_visited);

    std::cout << "[PASS] test_a_star_grid (A* visited " << astar_res.nodes_visited
              << " vs Dijkstra " << dijkstra_res.nodes_visited << " nodes)\n";
}

int main() {
    std::cout << "--- Running Shortest Path Tests ---\n";
    test_dijkstra_equivalency();
    test_a_star_grid();
    std::cout << "All Shortest Path tests PASSED!\n";
    return 0;
}
