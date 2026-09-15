#include "graphflow/core/dary_heap.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace graphflow::core;

void test_heap_basic() {
    IndexedDaryHeap<NodeId, EdgeWeight, 4> heap(10);
    assert(heap.empty());

    heap.push(1, 10.5);
    heap.push(2, 5.2);
    heap.push(3, 20.0);
    heap.push(4, 1.0);

    assert(heap.size() == 4);
    assert(heap.contains(1));
    assert(heap.contains(4));
    assert(!heap.contains(5));

    assert(heap.top().key == 4);
    assert(heap.top().priority == 1.0);

    assert(heap.pop().key == 4);
    assert(heap.pop().key == 2);
    assert(heap.pop().key == 1);
    assert(heap.pop().key == 3);

    assert(heap.empty());
    cout << "test_heap_basic passed\n";
}

void test_decrease_key() {
    IndexedDaryHeap<NodeId, EdgeWeight, 4> heap(10);
    heap.push(1, 50.0);
    heap.push(2, 30.0);
    heap.push(3, 40.0);

    assert(heap.top().key == 2);

    heap.push_or_decrease_key(1, 10.0);
    assert(heap.top().key == 1);
    assert(heap.top().priority == 10.0);

    heap.push_or_decrease_key(3, 5.0);
    assert(heap.top().key == 3);
    assert(heap.top().priority == 5.0);

    auto top = heap.pop();
    assert(top.key == 3 && top.priority == 5.0);

    top = heap.pop();
    assert(top.key == 1);

    top = heap.pop();
    assert(top.key == 2);

    cout << "test_decrease_key passed\n";
}

void test_random_sorted_extract() {
    constexpr size_t N = 1000;
    IndexedDaryHeap<NodeId, double, 4> heap(N);

    mt19937 rng(1337);
    uniform_real_distribution<double> dist(0.0, 10000.0);

    vector<double> vals;
    for (size_t i = 0; i < N; ++i) {
        double v = dist(rng);
        vals.push_back(v);
        heap.push(static_cast<NodeId>(i), v);
    }

    sort(vals.begin(), vals.end());

    for (size_t i = 0; i < N; ++i) {
        auto entry = heap.pop();
        assert(abs(entry.priority - vals[i]) < 1e-9);
    }

    assert(heap.empty());
    cout << "test_random_sorted_extract passed\n";
}

int main() {
    test_heap_basic();
    test_decrease_key();
    test_random_sorted_extract();
    return 0;
}
