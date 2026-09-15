#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <concepts>
#include <new>
#include <mutex>
#include <utility>

namespace graphflow::core {

/**
 * @brief Cache-aligned Monotonic Bump Arena Allocator.
 * Delivers O(1) allocations with zero internal fragmentation and O(1) bulk reset.
 * Avoids malloc/free lock contention under high-frequency query workloads.
 */
class ArenaAllocator {
public:
    static constexpr size_t kDefaultBlockSize = 2 * 1024 * 1024; // 2 MB blocks
    static constexpr size_t kCacheLineAlignment = 64;

    explicit ArenaAllocator(size_t default_block_size = kDefaultBlockSize);
    ~ArenaAllocator();

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&& other) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;

    /**
     * @brief Allocates `bytes` aligned to `alignment`.
     * Time Complexity: O(1).
     */
    [[nodiscard]] void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t));

    /**
     * @brief Typed allocation helper.
     */
    template <typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return ::new (mem) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Allocates an array of T of length count without calling individual constructors.
     */
    template <typename T>
    [[nodiscard]] T* allocate_array(size_t count) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    /**
     * @brief Bulk deallocation of all objects in O(1) by resetting block pointers.
     * Keeps allocated blocks in cache to eliminate subsequent syscalls.
     */
    void reset() noexcept;

    [[nodiscard]] size_t total_allocated() const noexcept { return total_allocated_; }
    [[nodiscard]] size_t total_capacity() const noexcept { return total_capacity_; }
    [[nodiscard]] size_t num_blocks() const noexcept { return blocks_.size(); }

private:
    struct Block {
        uint8_t* memory{nullptr};
        size_t capacity{0};
        size_t used{0};

        explicit Block(size_t cap);
        ~Block();

        Block(Block&&) noexcept;
        Block& operator=(Block&&) noexcept;
        Block(const Block&) = delete;
        Block& operator=(const Block&) = delete;
    };

    void allocate_new_block(size_t min_capacity);

    size_t default_block_size_;
    size_t current_block_index_{0};
    size_t total_allocated_{0};
    size_t total_capacity_{0};
    std::vector<Block> blocks_;
};

/**
 * @brief Thread-safe per-thread arena access manager.
 */
class ThreadLocalArenaPool {
public:
    static ArenaAllocator& get_thread_arena();
};

/**
 * @brief C++ standard library compliant allocator wrapping ArenaAllocator.
 */
template <typename T>
class ArenaStlAllocator {
public:
    using value_type = T;

    ArenaStlAllocator() noexcept : arena_(&ThreadLocalArenaPool::get_thread_arena()) {}
    explicit ArenaStlAllocator(ArenaAllocator& arena) noexcept : arena_(&arena) {}

    template <typename U>
    ArenaStlAllocator(const ArenaStlAllocator<U>& other) noexcept : arena_(other.arena_) {}

    [[nodiscard]] T* allocate(size_t n) {
        if (n == 0) return nullptr;
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* /*p*/, size_t /*n*/) noexcept {
        // Monotonic arena deallocation is a no-op; bulk reset is performed on arena_.
    }

    template <typename U>
    bool operator==(const ArenaStlAllocator<U>& other) const noexcept {
        return arena_ == other.arena_;
    }

    template <typename U>
    bool operator!=(const ArenaStlAllocator<U>& other) const noexcept {
        return arena_ != other.arena_;
    }

    ArenaAllocator* arena_{nullptr};
};

} // namespace graphflow::core
