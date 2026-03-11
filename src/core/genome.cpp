#include "genome.h"

#include <algorithm>

namespace swarm::core {

bool Genome::empty() const noexcept {
    return parameter_indices.empty();
}

std::size_t Genome::parameter_count() const noexcept {
    return parameter_indices.size();
}

Genome Genome::random(std::size_t parameter_count, std::uint32_t ocean_size, swarm::util::Rng& rng) {
    Genome genome;
    genome.hidden_units = static_cast<std::uint32_t>(2 + rng.uniform_int(0, 4));
    genome.action_count = 3;
    genome.aggressiveness_gene = static_cast<float>(rng.uniform_real(0.1, 1.0));
    genome.confidence_gene = static_cast<float>(rng.uniform_real(0.1, 1.0));
    genome.parameter_indices.reserve(parameter_count);

    const std::uint32_t bounded_ocean_size = std::max<std::uint32_t>(ocean_size, 1);
    for (std::size_t i = 0; i < parameter_count; ++i) {
        genome.parameter_indices.push_back(static_cast<std::uint32_t>(rng.uniform_int(0, static_cast<int>(bounded_ocean_size - 1))));
    }

    return genome;
}

} // namespace swarm::core
