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

swarm::core::Swarm make_seeded_swarm(const PopulationConfig& config, swarm::util::Rng& rng) {
    const std::size_t first_agents = static_cast<std::size_t>(rng.next_int(2, 9));
    const std::size_t second_agents = static_cast<std::size_t>(rng.next_int(2, 9));
    auto swarm = swarm::core::Swarm(
        make_random_chromosome(first_agents, config.state_size, static_cast<std::uint32_t>(config.ocean_size), rng),
        make_random_chromosome(second_agents, config.state_size, static_cast<std::uint32_t>(config.ocean_size), rng),
        0);

    const std::uint64_t lifecycle_roll = static_cast<std::uint64_t>(rng.next_int<int>(0, 3));
    if (lifecycle_roll == 0) {
        swarm.set_hands_played(static_cast<std::uint64_t>(rng.next_int<int>(0, 9000)));
    } else if (lifecycle_roll == 1) {
        swarm.set_hands_played(static_cast<std::uint64_t>(rng.next_int<int>(10000, 45000)));
    } else if (lifecycle_roll == 2) {
        swarm.set_hands_played(static_cast<std::uint64_t>(rng.next_int<int>(45001, 89999)));
    } else {
        swarm.set_hands_played(static_cast<std::uint64_t>(rng.next_int<int>(90000, 99999)));
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
        auto& swarm = population.swarms_.emplace_back(make_seeded_swarm(config, population.rng_));
        population.chip_manager_.inject(swarm, config.starting_bankroll);
        population.runtime_.emplace(swarm.id(), SwarmRuntimeState{});
    }

    population.rebuild_index();
    population.chip_manager_.require_invariants();
    return population;
}

const PopulationConfig& Population::config() const noexcept { return config_; }
swarm::core::Ocean& Population::ocean() noexcept { return ocean_; }
const swarm::core::Ocean& Population::ocean() const noexcept { return ocean_; }
std::vector<swarm::core::Swarm>& Population::swarms() noexcept { return swarms_; }
const std::vector<swarm::core::Swarm>& Population::swarms() const noexcept { return swarms_; }
const std::vector<swarm::core::Swarm>& Population::dead_swarms() const noexcept { return dead_swarms_; }
swarm::economy::ChipManager& Population::chip_manager() noexcept { return chip_manager_; }
const swarm::economy::ChipManager& Population::chip_manager() const noexcept { return chip_manager_; }
swarm::util::Rng& Population::rng() noexcept { return rng_; }
const swarm::util::Rng& Population::rng() const noexcept { return rng_; }
std::unordered_map<std::uint64_t, SwarmRuntimeState>& Population::runtime() noexcept { return runtime_; }
const std::unordered_map<std::uint64_t, SwarmRuntimeState>& Population::runtime() const noexcept { return runtime_; }

swarm::core::Swarm* Population::find_swarm(std::uint64_t swarm_id) noexcept {
    const auto it = id_to_index_.find(swarm_id);
    if (it != id_to_index_.end()) {
        return &swarms_[it->second];
    }
    for (auto& swarm : dead_swarms_) {
        if (swarm.id() == swarm_id) {
            return &swarm;
        }
    }
    return nullptr;
}

const swarm::core::Swarm* Population::find_swarm(std::uint64_t swarm_id) const noexcept {
    const auto it = id_to_index_.find(swarm_id);
    if (it != id_to_index_.end()) {
        return &swarms_[it->second];
    }
    for (const auto& swarm : dead_swarms_) {
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
    rebuild_index();
}

std::vector<swarm::core::Swarm*> Population::living_offspring_of(std::uint64_t parent_id) {
    std::vector<swarm::core::Swarm*> result;
    auto it = offspring_links_.find(parent_id);
    if (it == offspring_links_.end()) {
        return result;
    }

    for (const auto child_id : it->second) {
        auto active_it = id_to_index_.find(child_id);
        if (active_it != id_to_index_.end()) {
            auto& child = swarms_[active_it->second];
            if (child.alive()) {
                result.push_back(&child);
            }
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

void Population::rebuild_index() {
    id_to_index_.clear();
    id_to_index_.reserve(swarms_.size());
    for (std::size_t i = 0; i < swarms_.size(); ++i) {
        id_to_index_[swarms_[i].id()] = i;
    }
}

void Population::prune_dead() {
    std::size_t write = 0;
    for (std::size_t read = 0; read < swarms_.size(); ++read) {
        if (swarms_[read].alive()) {
            if (write != read) {
                swarms_[write] = std::move(swarms_[read]);
            }
            ++write;
        } else {
            dead_swarms_.push_back(std::move(swarms_[read]));
        }
    }
    swarms_.resize(write);
    rebuild_index();
}

} // namespace swarm::sim
