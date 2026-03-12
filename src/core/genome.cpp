#include "genome.h"

#include <algorithm>

namespace swarm::core {
namespace {

std::vector<std::uint32_t> make_ordered_indices(std::size_t count, std::uint32_t ocean_size, swarm::util::Rng& rng) {
    const std::uint32_t bounded_ocean_size = std::max<std::uint32_t>(ocean_size, 1);
    std::vector<std::uint32_t> indices;
    indices.reserve(count);

    std::uint32_t cursor = static_cast<std::uint32_t>(rng.uniform_int(0, static_cast<int>(bounded_ocean_size - 1)));
    for (std::size_t i = 0; i < count; ++i) {
        indices.push_back(cursor);
        const auto stride = static_cast<std::uint32_t>(1 + rng.uniform_int(0, 5));
        cursor = static_cast<std::uint32_t>((cursor + stride) % bounded_ocean_size);
    }
    return indices;
}

std::vector<std::uint32_t> shift_indices(const std::vector<std::uint32_t>& source, std::uint32_t shift, std::uint32_t ocean_size) {
    const std::uint32_t bounded_ocean_size = std::max<std::uint32_t>(ocean_size, 1);
    std::vector<std::uint32_t> shifted;
    shifted.reserve(source.size());
    for (const auto index : source) {
        shifted.push_back(static_cast<std::uint32_t>((index + shift) % bounded_ocean_size));
    }
    return shifted;
}

} // namespace

bool Genome::empty() const noexcept {
    return sensory_indices.empty() && hidden_indices.empty() && action_indices.empty() && modulation_indices.empty();
}

std::size_t Genome::parameter_count() const noexcept {
    return sensory_indices.size() + hidden_indices.size() + action_indices.size() + modulation_indices.size();
}

Genome Genome::random(std::size_t state_size, std::uint32_t ocean_size, swarm::util::Rng& rng, std::uint32_t chromosome_agents) {
    Genome genome;
    genome.hidden_units = static_cast<std::uint32_t>(2 + rng.uniform_int(0, 4));
    genome.action_count = 3;
    genome.ocean_window_radius = static_cast<std::uint32_t>(1 + rng.uniform_int(0, 2));
    genome.chromosome_agent_count = chromosome_agents == 0 ? static_cast<std::uint32_t>(2 + rng.uniform_int(0, 7)) : chromosome_agents;
    genome.alpha_bias = static_cast<float>(rng.uniform_real(0.1, 0.95));
    genome.risk_gene = static_cast<float>(rng.uniform_real(0.1, 1.0));
    genome.confidence_gene = static_cast<float>(rng.uniform_real(0.1, 1.0));
    genome.honesty_gene = static_cast<float>(rng.uniform_real(0.0, 1.0));
    genome.skepticism_gene = static_cast<float>(rng.uniform_real(0.0, 1.0));

    genome.sensory_indices = make_ordered_indices(state_size, ocean_size, rng);
    genome.hidden_indices = make_ordered_indices(static_cast<std::size_t>(genome.hidden_units) * 2, ocean_size, rng);
    genome.action_indices = make_ordered_indices(static_cast<std::size_t>(genome.action_count) * 2, ocean_size, rng);
    genome.modulation_indices = make_ordered_indices(4, ocean_size, rng);
    return genome;
}

Genome Genome::derive_agent_variant(std::size_t slot, std::uint32_t ocean_size) const {
    Genome variant = *this;
    const std::uint32_t shift = static_cast<std::uint32_t>((slot * 3) % std::max<std::uint32_t>(ocean_size, 1));
    variant.sensory_indices = shift_indices(sensory_indices, shift, ocean_size);
    variant.hidden_indices = shift_indices(hidden_indices, shift + 1, ocean_size);
    variant.action_indices = shift_indices(action_indices, shift + 2, ocean_size);
    variant.modulation_indices = shift_indices(modulation_indices, shift + 3, ocean_size);
    variant.alpha_bias = std::clamp(alpha_bias + static_cast<float>(slot) * 0.03f, 0.0f, 1.0f);
    variant.risk_gene = std::clamp(risk_gene + static_cast<float>(slot % 3) * 0.05f, 0.0f, 1.0f);
    variant.confidence_gene = std::clamp(confidence_gene - static_cast<float>(slot % 2) * 0.04f, 0.0f, 1.0f);
    variant.honesty_gene = std::clamp(honesty_gene - static_cast<float>(slot % 3) * 0.03f, 0.0f, 1.0f);
    variant.skepticism_gene = std::clamp(skepticism_gene + static_cast<float>(slot % 3) * 0.03f, 0.0f, 1.0f);
    return variant;
}

} // namespace swarm::core
