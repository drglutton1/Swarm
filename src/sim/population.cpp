#include "population.h"

#include <stdexcept>

#include "../core/chromosome.h"
#include "../core/genome.h"

namespace swarm::sim {
namespace {

swarm::core::Chromosome make_random_chromosome(
    std::size_t agent_count,
    std::size_t state_size,
    std::uint32_t ocean_size,
    swarm::util::Rng& rng) {
    const swarm::core::DecoderConfig config{state_size, 4, 3};
    auto genome = swarm::core::Genome::random(state_size, ocean_size, rng, static_cast<std::uint32_t>(agent_count));
    return swarm::core::Chromosome::spawn_from_blueprint(genome, agent_count, config, ocean_size);
}

swarm::core::Swarm make_seeded_swarm(std::size_t index, const PopulationConfig& config, swarm::util::Rng& rng) {
    const std::size_t first_agents = 2 + ((index * 3) % 4);
    const std::size_t second_agents = 2 + ((index * 5 + 1) % 4);
    swarm::core::Swarm swarm(
        make_random_chromosome(first_agents, config.state_size, static_cast<std::uint32_t>(config.ocean_size), rng),
        make_random_chromosome(second_agents, config.state_size, static_cast<std::uint32_t>(config.ocean_size), rng),
        0);

    const std::uint64_t lifecycle_band = static_cast<std::uint64_t>(index % 4);
    if (lifecycle_band == 0) {
        swarm.set_hands_played(2000 + static_cast<std::uint64_t>(index) * 37ULL);
    } else if (lifecycle_band == 1) {
        swarm.set_hands_played(15000 + static_cast<std::uint64_t>(index) * 41ULL);
    } else if (lifecycle_band == 2) {
        swarm.set_hands_played(42000 + static_cast<std::uint64_t>(index) * 29ULL);
    } else {
        swarm.set_hands_played(91000 + static_cast<std::uint64_t>(index) * 11ULL);
    }
    return swarm;
}

} // namespace

Population::Population(PopulationConfig config)
    : config_(config), ocean_(config.ocean_size), rng_(config.seed) {}

Population Population::create_default(PopulationConfig config) {
    Population population(config);
    population.ocean_.initialize_uniform(-1.0f, 1.0f, population.rng_);
    population.ocean_.diffuse(0.20f);
    population.ocean_.decay(0.995f);

    population.swarms_.reserve(config.swarm_count);
    for (std::size_t i = 0; i < config.swarm_count; ++i) {
        auto& swarm = population.swarms_.emplace_back(make_seeded_swarm(i, config, population.rng_));
        population.chip_manager_.inject(swarm, config.starting_bankroll);
        population.runtime_.emplace(swarm.id(), SwarmRuntimeState{});
    }

    population.chip_manager_.require_invariants();
    return population;
}

const PopulationConfig& Population::config() const noexcept { return config_; }
swarm::core::Ocean& Population::ocean() noexcept { return ocean_; }
const swarm::core::Ocean& Population::ocean() const noexcept { return ocean_; }
std::vector<swarm::core::Swarm>& Population::swarms() noexcept { return swarms_; }
const std::vector<swarm::core::Swarm>& Population::swarms() const noexcept { return swarms_; }
swarm::economy::ChipManager& Population::chip_manager() noexcept { return chip_manager_; }
const swarm::economy::ChipManager& Population::chip_manager() const noexcept { return chip_manager_; }
swarm::util::Rng& Population::rng() noexcept { return rng_; }
const swarm::util::Rng& Population::rng() const noexcept { return rng_; }
std::unordered_map<std::uint64_t, SwarmRuntimeState>& Population::runtime() noexcept { return runtime_; }
const std::unordered_map<std::uint64_t, SwarmRuntimeState>& Population::runtime() const noexcept { return runtime_; }

swarm::core::Swarm* Population::find_swarm(std::uint64_t swarm_id) noexcept {
    for (auto& swarm : swarms_) {
        if (swarm.id() == swarm_id) {
            return &swarm;
        }
    }
    return nullptr;
}

const swarm::core::Swarm* Population::find_swarm(std::uint64_t swarm_id) const noexcept {
    for (const auto& swarm : swarms_) {
        if (swarm.id() == swarm_id) {
            return &swarm;
        }
    }
    return nullptr;
}

SwarmRuntimeState& Population::runtime_state(std::uint64_t swarm_id) {
    return runtime_.at(swarm_id);
}

const SwarmRuntimeState& Population::runtime_state(std::uint64_t swarm_id) const {
    return runtime_.at(swarm_id);
}

void Population::register_offspring(std::uint64_t parent_a, std::uint64_t parent_b, std::uint64_t child_id) {
    offspring_links_[parent_a].push_back(child_id);
    offspring_links_[parent_b].push_back(child_id);
    runtime_[child_id] = SwarmRuntimeState{};
}

std::vector<swarm::core::Swarm*> Population::living_offspring_of(std::uint64_t parent_id) {
    std::vector<swarm::core::Swarm*> result;
    auto it = offspring_links_.find(parent_id);
    if (it == offspring_links_.end()) {
        return result;
    }

    for (const auto child_id : it->second) {
        if (auto* child = find_swarm(child_id); child != nullptr && child->alive()) {
            result.push_back(child);
        }
    }
    return result;
}

void Population::refresh_ocean() {
    if (ocean_.empty()) {
        throw std::logic_error("population ocean cannot be empty");
    }
    ocean_.diffuse(0.15f);
    ocean_.decay(0.998f);
    const std::uint32_t deposit_index = static_cast<std::uint32_t>(rng_.next_int<std::uint32_t>(0, static_cast<std::uint32_t>(ocean_.size() - 1)));
    ocean_.deposit(deposit_index, rng_.next_real<float>(-0.05f, 0.10f));
}

} // namespace swarm::sim
