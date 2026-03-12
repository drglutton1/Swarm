#include "mutation.h"

#include <algorithm>

namespace swarm::evolution {
namespace {

void maybe_mutate_indices(std::vector<std::uint32_t>& indices, std::uint32_t ocean_size, swarm::util::Rng& rng, double rate) {
    const std::uint32_t bounded_ocean = std::max<std::uint32_t>(ocean_size, 1);
    for (auto& index : indices) {
        if (rng.uniform_real() < rate) {
            index = static_cast<std::uint32_t>(rng.uniform_int(0, static_cast<int>(bounded_ocean - 1)));
        }
    }
}

} // namespace

void mutate_genome(swarm::core::Genome& genome, std::uint32_t ocean_size, swarm::util::Rng& rng, const MutationPolicy& policy) {
    maybe_mutate_indices(genome.sensory_indices, ocean_size, rng, policy.point_mutation_rate);
    maybe_mutate_indices(genome.hidden_indices, ocean_size, rng, policy.point_mutation_rate);
    maybe_mutate_indices(genome.action_indices, ocean_size, rng, policy.point_mutation_rate);
    maybe_mutate_indices(genome.modulation_indices, ocean_size, rng, policy.point_mutation_rate);

    if (rng.uniform_real() < policy.meta_mutation_rate) {
        genome.alpha_bias = std::clamp(genome.alpha_bias + static_cast<float>(rng.uniform_real(-0.15, 0.15)), 0.0f, 1.0f);
    }
    if (rng.uniform_real() < policy.meta_mutation_rate) {
        genome.risk_gene = std::clamp(genome.risk_gene + static_cast<float>(rng.uniform_real(-0.15, 0.15)), 0.0f, 1.0f);
    }
    if (rng.uniform_real() < policy.meta_mutation_rate) {
        genome.confidence_gene = std::clamp(genome.confidence_gene + static_cast<float>(rng.uniform_real(-0.15, 0.15)), 0.0f, 1.0f);
    }
    if (rng.uniform_real() < policy.meta_mutation_rate) {
        genome.honesty_gene = std::clamp(genome.honesty_gene + static_cast<float>(rng.uniform_real(-0.15, 0.15)), 0.0f, 1.0f);
    }
    if (rng.uniform_real() < policy.meta_mutation_rate) {
        genome.skepticism_gene = std::clamp(genome.skepticism_gene + static_cast<float>(rng.uniform_real(-0.15, 0.15)), 0.0f, 1.0f);
    }

    if (rng.uniform_real() < policy.structural_mutation_rate) {
        const int next_hidden = static_cast<int>(genome.hidden_units) + rng.uniform_int(-1, 1);
        genome.hidden_units = static_cast<std::uint32_t>(std::clamp(next_hidden, 2, 6));
        genome.hidden_indices.resize(static_cast<std::size_t>(genome.hidden_units) * 2, 0);
        maybe_mutate_indices(genome.hidden_indices, ocean_size, rng, 1.0);
    }
    if (rng.uniform_real() < policy.structural_mutation_rate) {
        const int next_radius = static_cast<int>(genome.ocean_window_radius) + rng.uniform_int(-1, 1);
        genome.ocean_window_radius = static_cast<std::uint32_t>(std::clamp(next_radius, 1, 3));
    }
    if (rng.uniform_real() < policy.structural_mutation_rate) {
        const int next_agents = static_cast<int>(genome.chromosome_agent_count) + rng.uniform_int(-1, 1);
        genome.chromosome_agent_count = static_cast<std::uint32_t>(std::clamp(next_agents, 2, 9));
    }
}

void mutate_chromosome(swarm::core::Chromosome& chromosome, std::size_t state_size, std::uint32_t ocean_size, swarm::util::Rng& rng, const MutationPolicy& policy) {
    swarm::core::Genome blueprint = chromosome.blueprint();
    mutate_genome(blueprint, ocean_size, rng, policy);
    const auto config = chromosome.agents().front().config();
    chromosome = swarm::core::Chromosome::spawn_from_blueprint(blueprint, blueprint.chromosome_agent_count == 0 ? chromosome.size() : blueprint.chromosome_agent_count, config, ocean_size);
}

} // namespace swarm::evolution
