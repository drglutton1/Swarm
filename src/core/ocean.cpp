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

void Ocean::diffuse(float neighbor_mix) {
    if (values_.size() < 2) {
        return;
    }

    const float mix = std::clamp(neighbor_mix, 0.0f, 1.0f);
    std::vector<float> next(values_.size(), 0.0f);
    for (std::size_t i = 0; i < values_.size(); ++i) {
        const float left = values_[wrap_index(static_cast<std::int64_t>(i) - 1)];
        const float right = values_[wrap_index(static_cast<std::int64_t>(i) + 1)];
        next[i] = values_[i] * (1.0f - mix) + 0.5f * mix * (left + right);
    }
    values_.swap(next);
}

void Ocean::deposit(std::uint32_t index, float delta) {
    set(index, get(index) + delta);
}

std::uint32_t Ocean::wrap_index(std::int64_t index) const {
    if (values_.empty()) {
        throw std::out_of_range("cannot wrap index in empty ocean");
    }
    const auto span = static_cast<std::int64_t>(values_.size());
    const auto wrapped = ((index % span) + span) % span;
    return static_cast<std::uint32_t>(wrapped);
}

float Ocean::window_mean(std::uint32_t center, std::uint32_t radius) const {
    if (values_.empty()) {
        return 0.0f;
    }

    float total = 0.0f;
    const std::uint32_t width = radius * 2 + 1;
    for (std::uint32_t offset = 0; offset < width; ++offset) {
        const auto signed_offset = static_cast<std::int64_t>(offset) - static_cast<std::int64_t>(radius);
        total += values_[wrap_index(static_cast<std::int64_t>(center) + signed_offset)];
    }
    return total / static_cast<float>(width);
}

float Ocean::local_gradient(std::uint32_t center) const {
    if (values_.empty()) {
        return 0.0f;
    }
    const float left = values_[wrap_index(static_cast<std::int64_t>(center) - 1)];
    const float right = values_[wrap_index(static_cast<std::int64_t>(center) + 1)];
    return right - left;
}

const std::vector<float>& Ocean::values() const noexcept {
    return values_;
}

} // namespace swarm::core
