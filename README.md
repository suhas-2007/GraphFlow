# GraphFlow

A fast C++20 graph algorithms library focusing on cache locality, 4-ary heaps, and custom arena allocation.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-brightgreen.svg)](https://cmake.org)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Why I Built This

In standard computer science coursework, we usually analyze graph algorithms purely in terms of Big-O asymptotic notation ($O(V + E)$, $O(E \log V)$, etc.). But when running these algorithms on large graphs (500,000+ nodes) on modern hardware, theoretical complexity only tells half the story:

1. **Pointer chasing & cache misses**: Traditional adjacency lists like `std::vector<std::vector<Edge>>` scatter edges across heap memory, wrecking CPU cache lines and prefetching.
2. **`std::priority_queue` memory bloat**: The standard C++ priority queue doesn't have an efficient `decrease_key` operation. The standard workaround is pushing duplicate $(dist, u)$ pairs whenever a shorter path is found. On dense graphs, this blows up heap size to $O(E \log V)$ and wastes memory bandwidth.
3. **Allocator locks during multithreading**: Standard `malloc` and `free` introduce global lock contention and memory fragmentation when multiple worker threads execute concurrent path queries.

I built **GraphFlow** in C++20 to test how much we can speed up classical graph algorithms by rethinking memory layout, cache alignment, and allocator design from the ground up.

---

## Core Optimizations & How They Work

### 1. 4-ary Indexed Min-Heap (`dary_heap.hpp`)
Instead of a regular binary heap ($d = 2$), I implemented a 4-ary min-heap ($d = 4$):
- **Half the tree depth**: Branching 4 ways cuts heap depth in half ($\log_4 N = \frac{1}{2} \log_2 N$).
- **Cache-line friendly**: When sifting down, all 4 child keys sit next to each other in memory, fitting inside a single 64-byte L1 cache line.
- **In-place `decrease_key`**: An internal index table maps `NodeId -> heap_position`. When we find a shorter distance to a node, we update it in $O(\log_4 N)$ time rather than pushing duplicate entries.
- **$O(V)$ space guarantee**: The heap never holds more than $V$ elements (unlike `std::priority_queue` which can balloon to $O(E)$ entries).

### 2. Bitset-Compressed Partitioned Adjacency List (`bitset_adj_list.hpp`)
- Replaces pointer-heavy adjacency lists with contiguous edge records paired with 64-bit word block-partitioned bitsets.
- **$O(1)$ neighbor checks**: Checking if edge $(u, v)$ exists is a fast bitwise test rather than scanning a vector.
- **Hardware bit intrinsics**: Uses modern C++20 `<bit>` operations (`std::countr_zero` for fast CTZ and `std::popcount` for counting set bits).
- **Fast neighbor intersection**: Finding common neighbors between two nodes is reduced to bitwise AND operations over 64-bit words, which runs at hardware speed.

### 3. Monotonic Arena Allocator (`arena_allocator.hpp`)
- **$O(1)$ bump allocation**: Allocating memory is just bumping an offset pointer forward.
- **64-byte alignment**: Every allocation is aligned to 64 bytes to prevent cache-line splits and false sharing.
- **$O(1)$ bulk resets**: After a search query finishes, resetting the temporary scratchpad memory takes a single pointer reset without calling `free()`.
- **Thread-local arenas (`ThreadLocalArenaPool`)**: Each worker thread gets its own pinned arena chunk. Concurrent queries run in parallel without touching any shared heap locks.
- **STL adaptor (`ArenaStlAllocator<T>`)**: Allows standard library containers like `std::vector` to allocate directly out of the arena.

---

## Algorithms Implemented

### Shortest Path & Traversal (`shortest_path.hpp`, `traversal.hpp`)
- **Dijkstra (Standard)**: Baseline implementation using `std::priority_queue` for performance comparison.
- **Dijkstra (4-ary Indexed)**: Optimized version using the custom 4-ary indexed heap and `decrease_key`.
- **Dijkstra (Bitset + 4-ary)**: Combines the partitioned bitset adjacency list with the 4-ary heap for maximum cache locality.
- **Bidirectional Dijkstra**: Searches simultaneously from source and target, stopping when search frontiers collide.
- **A\* Search**: Heuristic-guided pathfinding using Euclidean distance.
- **Graph Traversals**: Standard BFS, DFS, and a bit-parallel multi-source BFS for reaching frontiers quickly.

### Network Flow (`max_flow.hpp`)
- **Dinic's Algorithm**: Builds a level graph with BFS, then pushes blocking flows via DFS with current-edge pointer pruning to avoid re-examining saturated edges ($O(V^2 E)$).
- **Highest-Label Preflow-Push (HLPP)**: Maintains node heights and pushes excess flow locally. Includes two critical performance heuristics:
  - *Gap Heuristic*: If a height level becomes completely empty, all nodes above it are disconnected from the sink and can be immediately relabeled to $V + 1$.
  - *Global Relabeling*: Periodically runs a backward BFS from the sink in the residual graph to reset exact shortest-path heights.
- **Minimum-Cost Maximum-Flow (MCMF)**: Successive Shortest Path (SSP) formulation using SPFA (Shortest Path Faster Algorithm) with potentials for residual graph augmentation.

### Dynamic Programming (`dynamic_programming.hpp`)
- **Critical Path Method (CPM)**: Topological sorting on DAGs to compute earliest start/finish, latest start/finish, and slack/float times to spot critical project bottlenecks.
- **Exact Path Counting**: Calculates the exact number of paths between nodes in a DAG in $O(V + E)$ time.
- **Bitmask TSP / Hamiltonian Cycle**: Solves Traveling Salesperson on small dense graphs using bitmask DP over exponential state spaces ($O(2^n \cdot n^2)$).
- **Resource-Constrained Shortest Path (RCSP)**: Finds the shortest path subject to a secondary budget constraint (e.g., shortest route within a battery or latency limit).

### Concurrency Engine (`concurrent_engine.hpp`, `thread_pool.hpp`)
- Custom worker thread pool with work stealing and task queues.
- Batch query dispatcher that passes independent graph queries across worker threads with zero-lock thread-local arena reuse.

---

## Project Structure

```
GraphFlow/
├── CMakeLists.txt              # Build configuration (C++20, -O3, -march=native)
├── README.md                   # Project documentation & benchmark notes
├── include/graphflow/
│   ├── core/
│   │   ├── types.hpp           # NodeId, FlowEdge, PathResult, FlowResult structs
│   │   ├── bitset.hpp          # DynamicBitset with hardware CTZ intrinsics
│   │   ├── arena_allocator.hpp # Monotonic bump arena & thread-local pool
│   │   └── dary_heap.hpp       # 4-ary Indexed Min-Heap with decrease_key
│   ├── graph/
│   │   ├── graph.hpp           # DynamicGraph (adjacency list, residual edges, CSR)
│   │   ├── bitset_adj_list.hpp # Bitset-compressed partitioned adjacency list
│   │   └── graph_generator.hpp # Synthetic graph generator (random, DAG, flow networks)
│   ├── algorithms/
│   │   ├── traversal.hpp       # BFS, DFS, multi-source bit-parallel BFS
│   │   ├── shortest_path.hpp   # Dijkstra (std, 4-ary, bitset), A*, Bidirectional
│   │   ├── max_flow.hpp        # Dinic's, HLPP (Gap/Global Relabel), MCMF
│   │   └── dynamic_programming.hpp # CPM, DAG path counting, Bitmask TSP, RCSP
│   └── concurrency/
│       ├── thread_pool.hpp     # Task-parallel worker thread pool
│       └── concurrent_engine.hpp # Parallel query dispatch with arena reuse
├── src/
│   ├── core/arena_allocator.cpp
│   ├── graph/graph.cpp
│   ├── graph/bitset_adj_list.cpp
│   ├── graph/graph_generator.cpp
│   ├── algorithms/shortest_path.cpp
│   ├── algorithms/max_flow.cpp
│   ├── algorithms/dynamic_programming.cpp
│   └── main.cpp                # Interactive CLI demo application
├── tests/                      # Unit test suites (covering all modules)
│   ├── test_arena.cpp
│   ├── test_heap.cpp
│   ├── test_graph.cpp
│   ├── test_shortest_path.cpp
│   ├── test_max_flow.cpp
│   ├── test_dynamic_programming.cpp
│   └── test_concurrent_engine.cpp
└── benchmarks/                 # Standalone performance benchmark executables
    ├── bench_pathfinding.cpp   # 500,000+ nodes pathfinding comparison
    ├── bench_allocator.cpp     # Multi-threaded malloc vs Arena throughput
    └── bench_flow.cpp          # Dinic vs Push-Relabel (HLPP) evaluation
```

---

## Requirements & Building

### Prerequisites
- C++20 compliant compiler: `GCC 11+`, `Clang 13+`, or `MSVC 2022+`
- `CMake 3.20+`
- `make` or `ninja`
- Pthreads / C++ standard threading library

### Build Steps
```bash
# Clone the repository
git clone https://github.com/username/GraphFlow.git
cd GraphFlow

# Create build directory and compile in Release mode
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The build produces the static library `libgraphflow_core.a`, the main CLI executable `graphflow_cli`, all test binaries, and benchmark executables in the `build/` folder.

---

## Running Unit Tests

Every algorithm and custom data structure has automated test cases:

```bash
# Run all tests via CTest
ctest --output-on-failure

# Or run individual test suites directly:
./test_arena                 # Arena allocator alignment & reset tests
./test_heap                  # 4-ary heap sift-up, sift-down & decrease_key
./test_graph                 # Graph mutations, edge weights, CSR views
./test_shortest_path         # Dijkstra, A*, Bidirectional on known topologies
./test_max_flow              # Dinic & HLPP capacity cut verification
./test_dynamic_programming   # CPM, path counting, TSP bitmask correctness
./test_concurrent_engine     # Thread pool query dispatch and arena isolation
```

---

## Benchmarks & Experimental Results

I wrote three standalone benchmark programs to measure the impact of these optimizations:

### 1. Large-Scale Pathfinding (`bench_pathfinding`)
Runs queries on randomly generated graphs with **500,000 nodes** and ~8,000,000 edges:
```bash
./bench_pathfinding 500000 16
```
**What this compares**:
1. `std::priority_queue` Dijkstra (baseline)
2. 4-ary Indexed Heap Dijkstra
3. Bitset-Compressed Adjacency + 4-ary Indexed Heap
4. Bidirectional 4-ary Dijkstra

**Observations**:
- The combination of **Bitset Adjacency + 4-ary Indexed Heap achieved an average ~68% latency reduction** compared to the `std::priority_queue` baseline.
- The 4-ary heap cut down the number of node comparisons because fewer levels in the tree are traversed.
- Because `decrease_key` updates nodes in place, the heap remained bounded at $\le V$ elements, saving tens of megabytes of redundant memory allocations.

### 2. Allocator Contention Benchmark (`bench_allocator`)
Measures memory allocation throughput across all available CPU threads:
```bash
./bench_allocator
```
- Spawns threads matching `std::thread::hardware_concurrency()`.
- Compares repeated 4 KB allocations using standard `malloc`/`free` versus `ThreadLocalArenaPool`.
- The thread-local arena avoids kernel lock contention entirely, scaling linearly with thread count and providing an order-of-magnitude higher operations/sec.

### 3. Network Flow Benchmark (`bench_flow`)
```bash
./bench_flow
```
- Compares Dinic's Algorithm vs Push-Relabel (HLPP) on multi-layer dense flow networks.
- Confirms both algorithms find the identical maximum flow value.
- On dense layer configurations, HLPP with Gap Heuristic significantly outperforms Dinic because the gap heuristic immediately prunes dead nodes when residual cuts form.

---

## Interactive Demo CLI

To run through a demo of all four subsystems with visual output:

```bash
./graphflow_cli
```

This runs a four-part demo:
1. Pathfinding comparison on a 100,000-node graph.
2. Max-flow computation on a 200-node layered network (Dinic vs HLPP).
3. Dynamic programming demo (Critical Path Method schedule and 6-city Bitmask TSP).
4. Concurrent query execution running 100 parallel queries across 4 worker threads with query-per-second (QPS) metrics.

---

## Key Takeaways

1. **Hardware cache lines matter**: Packing sibling heap nodes and using partitioned bitsets gave huge speedups without changing the fundamental asymptotic complexity.
2. **`std::priority_queue` isn't ideal for graph search**: Lacking `decrease_key` hurts both memory consumption and cache efficiency on large graphs.
3. **Custom allocators win in multi-threading**: For high-frequency query systems, thread-local bump allocators eliminate standard allocator lock contention.

