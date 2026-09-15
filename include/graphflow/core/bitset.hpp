#pragma once

#include <cstdint>
#include <vector>
#include <bit>
#include <cstddef>
#include <algorithm>
#include <cstring>

namespace graphflow::core {

/**
 * @brief High-performance 64-bit word-aligned Dynamic Bitset.
 * Optimized for word-level SIMD-like operations and fast iteration over set bits
 * via hardware CTZ (count trailing zeros) intrinsics (std::countr_zero).
 */
class DynamicBitset {
public:
    using WordType = uint64_t;
    static constexpr size_t kBitsPerWord = sizeof(WordType) * 8;

    DynamicBitset() = default;

    explicit DynamicBitset(size_t num_bits, bool init_value = false)
        : num_bits_(num_bits),
          words_((num_bits + kBitsPerWord - 1) / kBitsPerWord, init_value ? ~WordType{0} : WordType{0}) {
        sanitize_tail();
    }

    void resize(size_t num_bits, bool init_value = false) {
        num_bits_ = num_bits;
        size_t new_words = (num_bits + kBitsPerWord - 1) / kBitsPerWord;
        words_.resize(new_words, init_value ? ~WordType{0} : WordType{0});
        sanitize_tail();
    }

    void reset() noexcept {
        std::fill(words_.begin(), words_.end(), WordType{0});
    }

    void set_all() noexcept {
        std::fill(words_.begin(), words_.end(), ~WordType{0});
        sanitize_tail();
    }

    [[nodiscard]] inline bool test(size_t bit_idx) const noexcept {
        if (bit_idx >= num_bits_) return false;
        return (words_[bit_idx / kBitsPerWord] & (WordType{1} << (bit_idx % kBitsPerWord))) != 0;
    }

    inline void set(size_t bit_idx) noexcept {
        if (bit_idx < num_bits_) {
            words_[bit_idx / kBitsPerWord] |= (WordType{1} << (bit_idx % kBitsPerWord));
        }
    }

    inline void reset(size_t bit_idx) noexcept {
        if (bit_idx < num_bits_) {
            words_[bit_idx / kBitsPerWord] &= ~(WordType{1} << (bit_idx % kBitsPerWord));
        }
    }

    inline void flip(size_t bit_idx) noexcept {
        if (bit_idx < num_bits_) {
            words_[bit_idx / kBitsPerWord] ^= (WordType{1} << (bit_idx % kBitsPerWord));
        }
    }

    [[nodiscard]] size_t count() const noexcept {
        size_t total = 0;
        for (WordType w : words_) {
            total += static_cast<size_t>(std::popcount(w));
        }
        return total;
    }

    [[nodiscard]] bool none() const noexcept {
        for (WordType w : words_) {
            if (w != 0) return false;
        }
        return true;
    }

    [[nodiscard]] bool any() const noexcept {
        return !none();
    }

    [[nodiscard]] size_t size() const noexcept {
        return num_bits_;
    }

    [[nodiscard]] size_t num_words() const noexcept {
        return words_.size();
    }

    [[nodiscard]] const WordType* data() const noexcept {
        return words_.data();
    }

    [[nodiscard]] WordType* data() noexcept {
        return words_.data();
    }

    // Fast bitwise operations
    DynamicBitset& operator&=(const DynamicBitset& other) noexcept {
        size_t n = std::min(words_.size(), other.words_.size());
        for (size_t i = 0; i < n; ++i) {
            words_[i] &= other.words_[i];
        }
        for (size_t i = n; i < words_.size(); ++i) {
            words_[i] = 0;
        }
        return *this;
    }

    DynamicBitset& operator|=(const DynamicBitset& other) noexcept {
        size_t n = std::min(words_.size(), other.words_.size());
        for (size_t i = 0; i < n; ++i) {
            words_[i] |= other.words_[i];
        }
        return *this;
    }

    DynamicBitset& operator^=(const DynamicBitset& other) noexcept {
        size_t n = std::min(words_.size(), other.words_.size());
        for (size_t i = 0; i < n; ++i) {
            words_[i] ^= other.words_[i];
        }
        return *this;
    }

    /**
     * @brief High-efficiency set-bit visitor.
     * Iterates only over indices where bit is 1, skipping entire 64-bit zero blocks.
     */
    template <typename Callback>
    void for_each_set_bit(Callback&& cb) const {
        for (size_t w_idx = 0; w_idx < words_.size(); ++w_idx) {
            WordType w = words_[w_idx];
            while (w != 0) {
                int bit_pos = std::countr_zero(w);
                size_t index = w_idx * kBitsPerWord + static_cast<size_t>(bit_pos);
                if (index >= num_bits_) break;
                cb(index);
                w &= (w - 1); // Clear least-significant set bit
            }
        }
    }

private:
    void sanitize_tail() noexcept {
        if (num_bits_ % kBitsPerWord != 0 && !words_.empty()) {
            WordType mask = (WordType{1} << (num_bits_ % kBitsPerWord)) - 1;
            words_.back() &= mask;
        }
    }

    size_t num_bits_{0};
    std::vector<WordType> words_;
};

} // namespace graphflow::core
