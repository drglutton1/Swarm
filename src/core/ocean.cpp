#include "ocean.h"

#include <algorithm>
#include <stdexcept>

namespace swarm::core {

Ocean::Ocean(std::size_t size, float initial_value) : values_(size, initial_value) {}

std::size_t Ocean::size() const noexcept {
    return values_.size();
}

bool Ocean::empty() const noexcept {
    return values_.empty();
}

float Ocean::get(std::uint32_t index) const {
    if (index >= values_.size()) {
        throw std::out_of_range("ocean index out of range");
    }
    return values_[index];
}

void Ocean::set(std::uint32_t index, float value) {
    if (index >= values_.size()) {
        throw std::out_of_range("ocean index out of range");
    }
    values_[index] = value;
}

void Ocean::fill(float value) {
    std::fill(values_.begin(), values_.end(), value);
}

void Ocean::initialize_uniform(float min_value, float max_value, swarm::util::Rng& rng) {
    for (float& value : values_) {
        value = static_cast<float>(rng.uniform_real(min_value, max_value));
    }
}

void Ocean::decay(float factor) {
    for (float& value : values_) {
        value *= factor;
    }
}

const std::vector<float>& Ocean::values() const noexcept {
    return values_;
}

} // namespace swarm::core
