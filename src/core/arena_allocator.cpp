#include "graphflow/core/arena_allocator.hpp"
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

namespace graphflow::core {

ArenaAllocator::Block::Block(size_t cap) : capacity(cap), used(0) {
#if defined(_MSC_VER)
    memory = static_cast<uint8_t*>(_aligned_malloc(capacity, kCacheLineAlignment));
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, kCacheLineAlignment, capacity) != 0) {
        ptr = nullptr;
    }
    memory = static_cast<uint8_t*>(ptr);
#endif
    if (!memory) {
        throw std::bad_alloc();
    }
}

ArenaAllocator::Block::~Block() {
    if (memory) {
#if defined(_MSC_VER)
        _aligned_free(memory);
#else
        free(memory);
#endif
        memory = nullptr;
    }
}

ArenaAllocator::Block::Block(Block&& other) noexcept
    : memory(other.memory), capacity(other.capacity), used(other.used) {
    other.memory = nullptr;
    other.capacity = 0;
    other.used = 0;
}

ArenaAllocator::Block& ArenaAllocator::Block::operator=(Block&& other) noexcept {
    if (this != &other) {
        if (memory) {
#if defined(_MSC_VER)
            _aligned_free(memory);
#else
            free(memory);
#endif
        }
        memory = other.memory;
        capacity = other.capacity;
        used = other.used;
        other.memory = nullptr;
        other.capacity = 0;
        other.used = 0;
    }
    return *this;
}

ArenaAllocator::ArenaAllocator(size_t default_block_size)
    : default_block_size_(std::max(default_block_size, size_t{4096})) {
    allocate_new_block(default_block_size_);
}

ArenaAllocator::~ArenaAllocator() = default;

ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept
    : default_block_size_(other.default_block_size_),
      current_block_index_(other.current_block_index_),
      total_allocated_(other.total_allocated_),
      total_capacity_(other.total_capacity_),
      blocks_(std::move(other.blocks_)) {
    other.current_block_index_ = 0;
    other.total_allocated_ = 0;
    other.total_capacity_ = 0;
}

ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
    if (this != &other) {
        default_block_size_ = other.default_block_size_;
        current_block_index_ = other.current_block_index_;
        total_allocated_ = other.total_allocated_;
        total_capacity_ = other.total_capacity_;
        blocks_ = std::move(other.blocks_);
        other.current_block_index_ = 0;
        other.total_allocated_ = 0;
        other.total_capacity_ = 0;
    }
    return *this;
}

void ArenaAllocator::allocate_new_block(size_t min_capacity) {
    size_t size = std::max(default_block_size_, min_capacity);
    blocks_.emplace_back(size);
    total_capacity_ += size;
}

void* ArenaAllocator::allocate(size_t bytes, size_t alignment) {
    if (bytes == 0) return nullptr;

    alignment = std::max(alignment, alignof(void*));

    while (current_block_index_ < blocks_.size()) {
        auto& block = blocks_[current_block_index_];
        uintptr_t current_addr = reinterpret_cast<uintptr_t>(block.memory + block.used);
        uintptr_t aligned_addr = (current_addr + (alignment - 1)) & ~(alignment - 1);
        size_t padding = aligned_addr - current_addr;

        if (block.used + padding + bytes <= block.capacity) {
            block.used += padding + bytes;
            total_allocated_ += padding + bytes;
            return reinterpret_cast<void*>(aligned_addr);
        }

        // Advance to next block or create one
        current_block_index_++;
    }

    // Need a new block capable of holding at least bytes + alignment
    allocate_new_block(bytes + alignment + default_block_size_);
    current_block_index_ = blocks_.size() - 1;

    auto& block = blocks_[current_block_index_];
    uintptr_t current_addr = reinterpret_cast<uintptr_t>(block.memory);
    uintptr_t aligned_addr = (current_addr + (alignment - 1)) & ~(alignment - 1);
    size_t padding = aligned_addr - current_addr;

    block.used = padding + bytes;
    total_allocated_ += padding + bytes;
    return reinterpret_cast<void*>(aligned_addr);
}

void ArenaAllocator::reset() noexcept {
    for (auto& block : blocks_) {
        block.used = 0;
    }
    current_block_index_ = 0;
    total_allocated_ = 0;
}

ArenaAllocator& ThreadLocalArenaPool::get_thread_arena() {
    thread_local ArenaAllocator thread_arena(4 * 1024 * 1024); // 4MB default per thread
    return thread_arena;
}

} // namespace graphflow::core
