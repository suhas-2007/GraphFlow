#include "graphflow/core/arena_allocator.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>

using namespace std;
using namespace graphflow::core;

void test_basic_allocation() {
    ArenaAllocator arena(1024 * 1024);
    void* p1 = arena.allocate(128, 64);
    assert(p1 != nullptr);
    assert(reinterpret_cast<uintptr_t>(p1) % 64 == 0);

    void* p2 = arena.allocate(256, 64);
    assert(p2 != nullptr);
    assert(reinterpret_cast<uintptr_t>(p2) % 64 == 0);
    assert(p2 > p1);

    int* val = arena.create<int>(42);
    assert(*val == 42);

    int* arr = arena.allocate_array<int>(100);
    for (int i = 0; i < 100; ++i) arr[i] = i * i;
    for (int i = 0; i < 100; ++i) assert(arr[i] == i * i);

    size_t alloc_before = arena.total_allocated();
    assert(alloc_before > 0);

    arena.reset();
    assert(arena.total_allocated() == 0);

    void* p3 = arena.allocate(128, 64);
    assert(p3 != nullptr);
    cout << "test_basic_allocation passed\n";
}

void test_stl_adaptor() {
    ArenaAllocator arena(1024 * 1024);
    {
        vector<int, ArenaStlAllocator<int>> vec((ArenaStlAllocator<int>(arena)));
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }
        assert(vec.size() == 1000);
        for (int i = 0; i < 1000; ++i) {
            assert(vec[i] == i);
        }
    }
    arena.reset();
    cout << "test_stl_adaptor passed\n";
}

void test_concurrent_arenas() {
    constexpr int num_threads = 4;
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            auto& arena = ThreadLocalArenaPool::get_thread_arena();
            for (int iter = 0; iter < 100; ++iter) {
                int* data = arena.allocate_array<int>(500);
                for (int i = 0; i < 500; ++i) data[i] = t * 1000 + i;
                for (int i = 0; i < 500; ++i) assert(data[i] == t * 1000 + i);
                arena.reset();
            }
        });
    }

    for (auto& th : threads) th.join();
    cout << "test_concurrent_arenas passed\n";
}

int main() {
    test_basic_allocation();
    test_stl_adaptor();
    test_concurrent_arenas();
    return 0;
}
