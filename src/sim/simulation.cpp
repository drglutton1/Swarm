#include "simulation.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "../economy/inheritance.h"
#include "../poker/table.h"
#include "../social/face.h"
#include "../social/info_exchange.h"
#include "../social/mate_selection.h"

namespace swarm::sim {

Simulation::Simulation(SimulationConfig config)
    : config_(config),
      population_(Population::create_default(config.population)),
      time_(config.time),
      activity_scheduler_(config.activity) {
    config_.reproduction.lifecycle = config_.lifecycle;
}

const SimulationConfig& Simulation::config() const noexcept { return config_; }
Population& Simulation::population() noexcept { return population_; }
const Population& Simulation::population() const noexcept { return population_; }
swarm::scheduler::TimeManager& Simulation::time() noexcept { return time_; }
const swarm::scheduler::TimeManager& Simulation::time() const noexcept { return time_; }
StatisticsCollector& Simulation::statistics() noexcept { return statistics_; }
const StatisticsCollector& Simulation::statistics() const noexcept { return statistics_; }

BlockResult Simulation::step_block() {
    population_.refresh_ocean();

    std::vector<swarm::scheduler::SchedulingPassEntry> entries;
    entries.reserve(population_.swarms().size());
    for (auto& swarm : population_.swarms()) {
        swarm::evolution::apply_mortality(swarm, config_.lifecycle);
        const auto lifecycle = swarm::evolution::evaluate(swarm, config_.lifecycle);
        auto& runtime = population_.runtime_state(swarm.id());
        runtime.activity = activity_scheduler_.next_state(swarm, lifecycle, time_.now(), runtime.activity.total_ticks == 0 ? nullptr : &runtime.activity);
        entries.push_back({&swarm, runtime.activity.mode, lifecycle});
    }

    std::size_t deaths_processed = 0;
    process_deaths(deaths_processed);
    const std::size_t offspring_born = attempt_social_reproduction();

    const auto plan = table_manager_.build_plan(entries);
    std::uint64_t hands_resolved = 0;
    for (std::size_t i = 0; i < plan.tables.size(); ++i) {
        hands_resolved += run_table_block(plan.tables[i], static_cast<std::uint64_t>(i));
    }

    const auto now = time_.advance_hand_blocks(1);
    statistics_.capture(population_, now, plan.tables.size(), hands_resolved, offspring_born, deaths_processed);

    return {now, plan.tables.size(), hands_resolved, offspring_born, deaths_processed,
        population_.chip_manager().invariants_hold() && population_.chip_manager().chips_in_play() == statistics_.latest().total_bankroll};
}

void Simulation::run_blocks(std::size_t block_count) {
    for (std::size_t i = 0; i < block_count; ++i) {
        step_block();
    }
}

std::string Simulation::latest_statistics_json() const {
    return statistics_.to_json();
}

void Simulation::process_deaths(std::size_t& death_count) {
    for (auto& swarm : population_.swarms()) {
        const auto lifecycle = swarm::evolution::evaluate(swarm, config_.lifecycle);
        if (lifecycle.alive || !swarm.alive()) {
            continue;
        }

        auto offspring = population_.living_offspring_of(swarm.id());
        swarm::economy::InheritanceService::distribute_on_death(swarm, offspring, population_.chip_manager());
        ++death_count;
    }
}

std::size_t Simulation::attempt_social_reproduction() {
    std::unordered_set<std::uint64_t> paired_this_block;
    std::size_t births = 0;

    for (auto& seeker : population_.swarms()) {
        const auto seeker_lifecycle = swarm::evolution::evaluate(seeker, config_.lifecycle);
        auto& runtime = population_.runtime_state(seeker.id());
        if (!seeker_lifecycle.reproduction_window_open || runtime.activity.mode == swarm::scheduler::ActivityMode::sleep || paired_this_block.count(seeker.id()) != 0) {
            continue;
        }

        std::vector<const swarm::core::Swarm*> candidates;
        std::vector<swarm::social::SocialKnowledge> knowledge;
        for (const auto& candidate : population_.swarms()) {
            if (candidate.id() == seeker.id()) {
                continue;
            }
            const auto candidate_lifecycle = swarm::evolution::evaluate(candidate, config_.lifecycle);
            if (!candidate_lifecycle.alive || paired_this_block.count(candidate.id()) != 0) {
                continue;
            }

            candidates.push_back(&candidate);
            const auto bankroll_response = swarm::social::request_private_info(seeker, candidate, swarm::social::PrivateInfoField::bankroll, population_.rng(), config_.lifecycle);
            const auto readiness_response = swarm::social::request_private_info(seeker, candidate, swarm::social::PrivateInfoField::reproductive_readiness, population_.rng(), config_.lifecycle);
            knowledge.push_back({
                swarm::social::make_public_face(candidate),
                swarm::social::interpret_response(seeker, bankroll_response),
                swarm::social::interpret_response(seeker, readiness_response),
                std::nullopt});
        }

        const auto selection = swarm::social::select_partner(seeker, candidates, knowledge, config_.lifecycle);
        if (selection.partner == nullptr) {
            continue;
        }

        auto* partner = population_.find_swarm(selection.partner->id());
        if (partner == nullptr || paired_this_block.count(partner->id()) != 0) {
            continue;
        }

        auto result = swarm::evolution::reproduce(seeker, *partner, config_.population.state_size,
            static_cast<std::uint32_t>(config_.population.ocean_size), population_.rng(), config_.reproduction);
        if (!result.success || !result.offspring.has_value()) {
            continue;
        }

        population_.chip_manager().inject(*result.offspring, config_.population.starting_bankroll);
        auto& child = population_.swarms().emplace_back(std::move(*result.offspring));
        population_.register_offspring(seeker.id(), partner->id(), child.id());
        population_.runtime_state(seeker.id()).offspring_born_in_sim += 1;
        population_.runtime_state(partner->id()).offspring_born_in_sim += 1;
        paired_this_block.insert(seeker.id());
        paired_this_block.insert(partner->id());
        ++births;
    }

    return births;
}

std::uint64_t Simulation::run_table_block(const swarm::scheduler::TableAssignment& assignment, std::uint64_t seed_offset) {
    if (assignment.swarm_ids.size() < 2) {
        return 0;
    }

    swarm::poker::Table table(
        static_cast<int>(assignment.swarm_ids.size()), 0, config_.small_blind, config_.big_blind, config_.rake_per_hand,
        config_.population.seed + assignment.table_id + seed_offset * 97ULL + time_.now().hand_block * 131ULL);

    std::vector<swarm::core::Swarm*> seated;
    seated.reserve(assignment.swarm_ids.size());
    for (std::size_t i = 0; i < assignment.swarm_ids.size(); ++i) {
        auto* swarm = population_.find_swarm(assignment.swarm_ids[i]);
        if (swarm == nullptr) {
            continue;
        }
        table.set_player_stack_for_test(i, swarm->bankroll());
        seated.push_back(swarm);
    }

    std::ostringstream sink;
    const auto summary = table.run(static_cast<int>(config_.hands_per_table_block), false, sink);
    for (std::size_t i = 0; i < seated.size() && i < summary.ending_stacks.size(); ++i) {
        auto* swarm = seated[i];
        const std::int64_t delta = summary.ending_stacks[i] - swarm->bankroll();
        if (delta > 0) {
            swarm->add_bankroll(delta);
        } else if (delta < 0) {
            swarm->remove_bankroll(-delta);
        }
        swarm->record_hands(static_cast<std::uint64_t>(summary.hands_played));
        auto& runtime = population_.runtime_state(swarm->id());
        runtime.total_hands_played_in_sim += static_cast<std::uint64_t>(summary.hands_played);
        runtime.tables_sat += 1;
    }
    return static_cast<std::uint64_t>(summary.hands_played) * static_cast<std::uint64_t>(assignment.swarm_ids.size());
}

} // namespace swarm::sim
