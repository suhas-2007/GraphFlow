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

struct QueryPair {
    core::NodeId source;
    core::NodeId target;
};

/**
 * @brief Concurrent Query Engine.
 * Executes batches of path queries and state-space evaluations in parallel.
 * Utilizes ThreadLocalArenaPool to achieve O(1) allocation overhead under high concurrency,
 * eliminating malloc/free lock contention.
 */
class ConcurrentEngine {
public:
    explicit ConcurrentEngine(size_t num_threads = std::thread::hardware_concurrency())
        : pool_(num_threads) {}

    /**
     * @brief Dispatches a batch of shortest path queries in parallel across worker threads.
     */
    std::vector<core::PathResult> execute_batch_queries(
        const graph::DynamicGraph& g,
        const std::vector<QueryPair>& queries
    ) {
        std::vector<std::future<core::PathResult>> futures;
        futures.reserve(queries.size());

        for (const auto& q : queries) {
            futures.push_back(pool_.submit([&g, src = q.source, dst = q.target]() {
                // Thread acquires thread-local arena
                auto& arena = core::ThreadLocalArenaPool::get_thread_arena();

                // Perform accelerated 4-ary indexed pathfinding
                core::PathResult res = algorithms::ShortestPath::dijkstra_indexed_4ary(g, src, dst);

                // O(1) bulk reset of thread arena scratchpad
                arena.reset();

                return res;
            }));
        }

        std::vector<core::PathResult> results;
        results.reserve(queries.size());
        for (auto& f : futures) {
            results.push_back(f.get());
        }

        return results;
    }

    /**
     * @brief Dispatches batch queries over BitsetAdjacencyList.
     */
    std::vector<core::PathResult> execute_batch_bitset_queries(
        const graph::BitsetAdjacencyList& g,
        const std::vector<QueryPair>& queries
    ) {
        std::vector<std::future<core::PathResult>> futures;
        futures.reserve(queries.size());

        for (const auto& q : queries) {
            futures.push_back(pool_.submit([&g, src = q.source, dst = q.target]() {
                auto& arena = core::ThreadLocalArenaPool::get_thread_arena();
                core::PathResult res = algorithms::ShortestPath::dijkstra_bitset(g, src, dst);
                arena.reset();
                return res;
            }));
        }

        std::vector<core::PathResult> results;
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
