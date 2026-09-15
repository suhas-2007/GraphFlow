#include "graphflow/core/arena_allocator.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cstdlib>

using namespace std;
using namespace std::chrono;
using namespace graphflow::core;

int main() {
    size_t num_threads = thread::hardware_concurrency();
    size_t num_queries_per_thread = 50000;
    size_t alloc_size_per_query = 4096; // 4 KB per query

    cout << "Allocator Benchmark: Standard malloc/free vs ArenaAllocator\n";
    cout << "Threads: " << num_threads << " | Operations/Thread: " << num_queries_per_thread
         << " | Allocation size: " << (alloc_size_per_query / 1024) << " KB\n";
    cout << "------------------------------------------------------------\n";

    // 1. Standard malloc/free
    {
        auto t_start = high_resolution_clock::now();

        vector<thread> workers;
        for (size_t t = 0; t < num_threads; ++t) {
            workers.emplace_back([&]() {
                for (size_t q = 0; q < num_queries_per_thread; ++q) {
                    volatile uint8_t* ptr = static_cast<uint8_t*>(malloc(alloc_size_per_query));
                    if (ptr) {
                        ptr[0] = 1;
                        ptr[alloc_size_per_query - 1] = 2;
                        free(const_cast<uint8_t*>(ptr));
                    }
                }
            });
        }
        for (auto& w : workers) w.join();

        auto t_end = high_resolution_clock::now();
        double ms = duration<double, milli>(t_end - t_start).count();
        double qps = (num_threads * num_queries_per_thread) / (ms / 1000.0);
        cout << "  malloc/free:    " << fixed << setprecision(2) << ms << " ms ("
             << setprecision(0) << qps << " ops/sec)\n";
    }

    // 2. ThreadLocalArenaPool
    {
        auto t_start = high_resolution_clock::now();

        vector<thread> workers;
        for (size_t t = 0; t < num_threads; ++t) {
            workers.emplace_back([&]() {
                auto& arena = ThreadLocalArenaPool::get_thread_arena();
                for (size_t q = 0; q < num_queries_per_thread; ++q) {
                    volatile uint8_t* ptr = static_cast<uint8_t*>(arena.allocate(alloc_size_per_query, 64));
                    if (ptr) {
                        ptr[0] = 1;
                        ptr[alloc_size_per_query - 1] = 2;
                    }
                    arena.reset();
                }
            });
        }
        for (auto& w : workers) w.join();

        auto t_end = high_resolution_clock::now();
        double ms = duration<double, milli>(t_end - t_start).count();
        double qps = (num_threads * num_queries_per_thread) / (ms / 1000.0);
        cout << "  ArenaAllocator: " << fixed << setprecision(2) << ms << " ms ("
             << setprecision(0) << qps << " ops/sec)\n";
    }

    return 0;
}
