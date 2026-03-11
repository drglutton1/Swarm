#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../util/rng.h"

namespace swarm::core {

struct Genome {
    std::vector<std::uint32_t> parameter_indices;
    std::uint32_t hidden_units {4};
    std::uint32_t action_count {3};
    float aggressiveness_gene {0.5f};
    float confidence_gene {0.5f};

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t parameter_count() const noexcept;

    static Genome random(std::size_t parameter_count, std::uint32_t ocean_size, swarm::util::Rng& rng);
};

} // namespace swarm::core
