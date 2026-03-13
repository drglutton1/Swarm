#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace swarm::util {
namespace detail {

inline std::uint64_t splitmix64(std::uint64_t& state) noexcept {
    std::uint64_t value = (state += 0x9E3779B97F4A7C15ULL);
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

inline std::uint64_t rotl(const std::uint64_t value, int shift) noexcept {
    return (value << shift) | (value >> (64 - shift));
}

} // namespace detail

class Rng {
public:
    explicit Rng(std::uint64_t seed = std::random_device{}()) {
        reseed(seed);
    }

    void reseed(std::uint64_t seed) noexcept {
        seed_ = seed;
        std::uint64_t sm_state = seed;
        for (auto& word : state_) {
            word = detail::splitmix64(sm_state);
        }
        if ((state_[0] | state_[1] | state_[2] | state_[3]) == 0) {
            state_[0] = 0x9E3779B97F4A7C15ULL;
        }
    }

    [[nodiscard]] std::uint64_t seed() const noexcept {
        return seed_;
    }

    [[nodiscard]] std::uint64_t next_u64() noexcept {
        const std::uint64_t result = detail::rotl(state_[1] * 5ULL, 7) * 9ULL;
        const std::uint64_t t = state_[1] << 17;

        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = detail::rotl(state_[3], 45);
        return result;
    }

    [[nodiscard]] std::uint32_t next_u32() noexcept {
        return static_cast<std::uint32_t>(next_u64() >> 32U);
    }

    [[nodiscard]] double next_double() noexcept {
        return static_cast<double>(next_u64() >> 11U) * (1.0 / 9007199254740992.0);
    }

    [[nodiscard]] float next_float() noexcept {
        return static_cast<float>(next_double());
    }

    template <typename T>
    T next_int(T min, T max) {
        static_assert(std::is_integral_v<T>, "next_int requires an integral type");
        if (max < min) {
            throw std::invalid_argument("rng integer bounds out of order");
        }

        using Unsigned = std::make_unsigned_t<T>;
        const auto min_signed = static_cast<std::int64_t>(min);
        const auto max_signed = static_cast<std::int64_t>(max);
        const auto span = static_cast<Unsigned>(max_signed - min_signed) + 1;

        if (span == 0) {
            return static_cast<T>(next_u64());
        }

        const std::uint64_t limit = std::numeric_limits<std::uint64_t>::max() -
            (std::numeric_limits<std::uint64_t>::max() % static_cast<std::uint64_t>(span));

        std::uint64_t value = 0;
        do {
            value = next_u64();
        } while (value >= limit);

        return static_cast<T>(min_signed + static_cast<std::int64_t>(value % static_cast<std::uint64_t>(span)));
    }

    template <typename T>
    T next_real(T min, T max) {
        static_assert(std::is_floating_point_v<T>, "next_real requires a floating-point type");
        if (max < min) {
            throw std::invalid_argument("rng real bounds out of order");
        }
        return static_cast<T>(min + (max - min) * next_double());
    }

    template <typename T>
    T uniform_int(T min, T max) {
        return next_int(min, max);
    }

    double uniform_real(double min = 0.0, double max = 1.0) {
        return next_real<double>(min, max);
    }

    template <typename It>
    void shuffle(It first, It last) {
        using diff_t = typename std::iterator_traits<It>::difference_type;
        const diff_t count = std::distance(first, last);
        if (count <= 1) {
            return;
        }
        for (diff_t i = count - 1; i > 0; --i) {
            const diff_t j = next_int<diff_t>(0, i);
            if (i != j) {
                std::iter_swap(first + i, first + j);
            }
        }
    }

private:
    std::uint64_t seed_ = 0;
    std::array<std::uint64_t, 4> state_ {0, 0, 0, 0};
};

} // namespace swarm::util
