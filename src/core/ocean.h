#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../util/rng.h"

namespace swarm::core {

class Ocean {
public:
    Ocean() = default;
    explicit Ocean(std::size_t size, float initial_value = 0.0f);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] float get(std::uint32_t index) const;
    void set(std::uint32_t index, float value);

    void fill(float value);
    void initialize_uniform(float min_value, float max_value, swarm::util::Rng& rng);
    void decay(float factor);

    [[nodiscard]] const std::vector<float>& values() const noexcept;

private:
    std::vector<float> values_;
};

} // namespace swarm::core
