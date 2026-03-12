#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../util/rng.h"

namespace swarm::core {

struct Genome {
    std::vector<std::uint32_t> sensory_indices;
    std::vector<std::uint32_t> hidden_indices;
    std::vector<std::uint32_t> action_indices;
    std::vector<std::uint32_t> modulation_indices;

    std::uint32_t hidden_units {4};
    std::uint32_t action_count {3};
    std::uint32_t ocean_window_radius {1};
    std::uint32_t chromosome_agent_count {2};
    float alpha_bias {0.5f};
    float risk_gene {0.5f};
    float confidence_gene {0.5f};
    float honesty_gene {0.5f};
    float skepticism_gene {0.5f};

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t parameter_count() const noexcept;

    static Genome random(std::size_t state_size, std::uint32_t ocean_size, swarm::util::Rng& rng, std::uint32_t chromosome_agent_count = 0);
    [[nodiscard]] Genome derive_agent_variant(std::size_t slot, std::uint32_t ocean_size) const;
};

} // namespace swarm::core
