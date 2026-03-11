#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

namespace swarm::util {

class Rng {
public:
    explicit Rng(std::uint64_t seed = std::random_device{}()) : engine_(seed) {}

    template <typename T>
    T uniform_int(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(engine_);
    }

    double uniform_real(double min = 0.0, double max = 1.0) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(engine_);
    }

    template <typename It>
    void shuffle(It first, It last) {
        std::shuffle(first, last, engine_);
    }

private:
    std::mt19937_64 engine_;
};

} // namespace swarm::util
