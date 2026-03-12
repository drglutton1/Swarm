#include "reproduction.h"

#include <algorithm>
#include <utility>

namespace swarm::evolution {
namespace {

const swarm::core::Chromosome& pick_parent_chromosome(const swarm::core::Swarm& parent, swarm::util::Rng& rng) {
    return rng.uniform_int(0, 1) == 0 ? parent.first_chromosome() : parent.second_chromosome();
}

} // namespace

bool pairing_parity_allowed(const swarm::core::Swarm& first, const swarm::core::Swarm& second) {
    const bool first_even = first.total_agents() % 2 == 0;
    const bool second_even = second.total_agents() % 2 == 0;
    return first_even != second_even;
}

ReproductionResult reproduce(
    swarm::core::Swarm& first,
    swarm::core::Swarm& second,
    std::size_t state_size,
    std::uint32_t ocean_size,
    swarm::util::Rng& rng,
    const ReproductionPolicy& policy) {
    apply_mortality(first, policy.lifecycle);
    apply_mortality(second, policy.lifecycle);

    if (!pairing_parity_allowed(first, second)) {
        return {false, "pairing requires one even-sized swarm and one odd-sized swarm", std::nullopt};
    }
    if (!can_reproduce(first, policy.lifecycle) || !can_reproduce(second, policy.lifecycle)) {
        return {false, "both parents must be alive, mature, off cooldown, and under offspring cap", std::nullopt};
    }

    const auto& first_source = pick_parent_chromosome(first, rng);
    const auto& second_source = pick_parent_chromosome(second, rng);
    auto crossed = maybe_crossover(first_source.blueprint(), second_source.blueprint(), rng, policy.crossover);
    mutate_genome(crossed.first, ocean_size, rng, policy.mutation);
    mutate_genome(crossed.second, ocean_size, rng, policy.mutation);

    crossed.first.chromosome_agent_count = std::clamp<std::uint32_t>(crossed.first.chromosome_agent_count == 0 ? static_cast<std::uint32_t>(first_source.size()) : crossed.first.chromosome_agent_count, 2, 9);
    crossed.second.chromosome_agent_count = std::clamp<std::uint32_t>(crossed.second.chromosome_agent_count == 0 ? static_cast<std::uint32_t>(second_source.size()) : crossed.second.chromosome_agent_count, 2, 9);

    const swarm::core::DecoderConfig config{state_size, 4, 3};
    swarm::core::Swarm child(
        swarm::core::Chromosome::spawn_from_blueprint(crossed.first, crossed.first.chromosome_agent_count, config, ocean_size),
        swarm::core::Chromosome::spawn_from_blueprint(crossed.second, crossed.second.chromosome_agent_count, config, ocean_size),
        swarm::core::Swarm::starting_bankroll);

    first.note_reproduction();
    second.note_reproduction();

    return {true, "ok", std::move(child)};
}

} // namespace swarm::evolution
