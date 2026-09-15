#pragma once

#include <vector>
#include <future>
#include <memory>
#include <atomic>
#include "graphflow/core/types.hpp"
#include "graphflow/core/arena_allocator.hpp"
#include "graphflow/graph/graph.hpp"
#include "graphflow/graph/bitset_adj_list.hpp"
#include "graphflow/algorithms/shortest_path.hpp"
#include "graphflow/concurrency/thread_pool.hpp"

namespace graphflow::concurrency {

using namespace core;
using graph::DynamicGraph;
using graph::BitsetAdjacencyList;

struct QueryPair {
    NodeId source;
    NodeId target;
};

// Parallel batch query dispatcher using thread-local arenas
class ConcurrentEngine {
public:
    explicit ConcurrentEngine(size_t num_threads = std::thread::hardware_concurrency())
        : pool_(num_threads) {}

    std::vector<PathResult> execute_batch_queries(
        const DynamicGraph& g,
        const std::vector<QueryPair>& queries
    ) {
        std::vector<std::future<PathResult>> futures;
        futures.reserve(queries.size());

        for (const auto& q : queries) {
            futures.push_back(pool_.submit([&g, src = q.source, dst = q.target]() {
                auto& arena = ThreadLocalArenaPool::get_thread_arena();
                PathResult res = algorithms::ShortestPath::dijkstra_indexed_4ary(g, src, dst);
                arena.reset();
                return res;
            }));
        }

        std::vector<PathResult> results;
        results.reserve(queries.size());
        for (auto& f : futures) {
            results.push_back(f.get());
        }

        return results;
    }

    std::vector<PathResult> execute_batch_bitset_queries(
        const BitsetAdjacencyList& g,
        const std::vector<QueryPair>& queries
    ) {
        std::vector<std::future<PathResult>> futures;
        futures.reserve(queries.size());

        for (const auto& q : queries) {
            futures.push_back(pool_.submit([&g, src = q.source, dst = q.target]() {
                auto& arena = ThreadLocalArenaPool::get_thread_arena();
                PathResult res = algorithms::ShortestPath::dijkstra_bitset(g, src, dst);
                arena.reset();
                return res;
            }));
        }

        std::vector<PathResult> results;
        results.reserve(queries.size());
        for (auto& f : futures) {
            results.push_back(f.get());
        }

        return results;
    }

    [[nodiscard]] size_t num_workers() const noexcept {
        return pool_.size();
    }

private:
    ThreadPool pool_;
};

} // namespace graphflow::concurrency
