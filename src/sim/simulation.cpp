#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "../economy/inheritance.h"
#include "../poker/table.h"
#include "../social/face.h"
#include "../social/info_exchange.h"
#include "../social/mate_selection.h"

namespace swarm::sim {
namespace {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float normalize_stack(std::int64_t value) {
    return clamp01(static_cast<float>(value) / static_cast<float>(swarm::core::Swarm::starting_bankroll * 2));
}

float street_progress(swarm::poker::DecisionContext::Street street) {
    switch (street) {
        case swarm::poker::DecisionContext::Street::preflop:
            return 0.0f;
        case swarm::poker::DecisionContext::Street::flop:
            return 0.33f;
        case swarm::poker::DecisionContext::Street::turn:
            return 0.66f;
        case swarm::poker::DecisionContext::Street::river:
            return 1.0f;
    }
    return 0.0f;
}

swarm::core::PokerStateVector make_state_vector(const swarm::poker::DecisionContext& context) {
    swarm::core::PokerStateVector state;
    const float stack = static_cast<float>(std::max<std::int64_t>(0, context.stack));
    const float to_call = static_cast<float>(std::max<std::int64_t>(0, context.to_call));
    const float pot = static_cast<float>(std::max<std::int64_t>(0, context.pot));
    const float active = static_cast<float>(std::max<std::size_t>(1, context.active_players));
    const float funded = static_cast<float>(std::max<std::size_t>(1, context.funded_players));

    state.features = {
        clamp01((stack <= 0.0f) ? 0.0f : to_call / std::max(stack, 1.0f)),
        clamp01((stack <= 0.0f) ? 0.0f : pot / std::max(stack, 1.0f)),
        clamp01(static_cast<float>(context.board.size()) / 5.0f),
        clamp01((funded <= 1.0f) ? 0.0f : (active - 1.0f) / (funded - 1.0f)),
        street_progress(context.street),
        normalize_stack(context.stack)};
    state.to_call = to_call;
    state.min_raise = static_cast<float>(std::max<std::int64_t>(0, context.min_raise_to - context.current_bet));
    state.pot = pot;
    state.stack = stack;
    return state;
}

swarm::poker::ForcedAction map_decision_to_action(
    const swarm::core::Decision& decision,
    const swarm::poker::DecisionContext& context) {
    switch (decision.action) {
        case swarm::core::ActionType::fold:
            if (context.to_call <= 0) {
                return {swarm::poker::ForcedAction::Type::check_call, 0};
            }
            return {swarm::poker::ForcedAction::Type::fold, 0};
        case swarm::core::ActionType::check_call:
            return {swarm::poker::ForcedAction::Type::check_call, 0};
        case swarm::core::ActionType::raise: {
            const std::int64_t max_raise_to = context.players[context.player_index].street_commitment + context.stack;
            const std::int64_t requested_raise = context.current_bet + static_cast<std::int64_t>(std::llround(decision.raise_amount));
            const std::int64_t target = std::clamp(requested_raise, context.min_raise_to, max_raise_to);
            if (target <= context.current_bet) {
                return {swarm::poker::ForcedAction::Type::check_call, 0};
            }
            return {swarm::poker::ForcedAction::Type::raise_to, target};
        }
    }
    return {swarm::poker::ForcedAction::Type::check_call, 0};
}

} // namespace

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

    for (auto& swarm : population_.swarms()) {
        swarm::evolution::apply_mortality(swarm, config_.lifecycle);
    }

    std::size_t deaths_processed = 0;
    process_deaths(deaths_processed);
    const std::size_t offspring_born = attempt_social_reproduction();

    std::vector<swarm::scheduler::SchedulingPassEntry> entries;
    entries.reserve(population_.swarms().size());
    for (auto& swarm : population_.swarms()) {
        const auto lifecycle = swarm::evolution::evaluate(swarm, config_.lifecycle);
        auto& runtime = population_.runtime_state(swarm.id());
        runtime.activity = activity_scheduler_.next_state(swarm, lifecycle, time_.now(), runtime.activity.total_ticks == 0 ? nullptr : &runtime.activity);
        entries.push_back({&swarm, runtime.activity.mode, lifecycle});
    }

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
    population_.prune_dead();
}

std::size_t Simulation::attempt_social_reproduction() {
    std::unordered_set<std::uint64_t> paired_this_block;
    std::size_t births = 0;

    std::vector<std::uint64_t> even_ids;
    std::vector<std::uint64_t> odd_ids;
    even_ids.reserve(population_.swarms().size());
    odd_ids.reserve(population_.swarms().size());
    for (const auto& swarm : population_.swarms()) {
        if (swarm.total_agents() % 2 == 0) {
            even_ids.push_back(swarm.id());
        } else {
            odd_ids.push_back(swarm.id());
        }
    }

    constexpr std::size_t candidate_sample_limit = 24;
    for (auto& seeker : population_.swarms()) {
        const auto seeker_lifecycle = swarm::evolution::evaluate(seeker, config_.lifecycle);
        auto& runtime = population_.runtime_state(seeker.id());
        if (!seeker_lifecycle.reproduction_window_open || runtime.activity.mode == swarm::scheduler::ActivityMode::sleep || paired_this_block.count(seeker.id()) != 0) {
            continue;
        }

        const auto& pool_ids = (seeker.total_agents() % 2 == 0) ? odd_ids : even_ids;
        if (pool_ids.empty()) {
            continue;
        }

        std::vector<const swarm::core::Swarm*> candidates;
        std::vector<swarm::social::SocialKnowledge> knowledge;
        candidates.reserve(std::min(candidate_sample_limit, pool_ids.size()));
        knowledge.reserve(std::min(candidate_sample_limit, pool_ids.size()));

        const std::size_t offset = pool_ids.empty() ? 0 : static_cast<std::size_t>(population_.rng().next_int<std::uint32_t>(0, static_cast<std::uint32_t>(pool_ids.size() - 1)));
        for (std::size_t attempt = 0; attempt < pool_ids.size() && candidates.size() < candidate_sample_limit; ++attempt) {
            const auto candidate_id = pool_ids[(offset + attempt) % pool_ids.size()];
            auto* candidate = population_.find_swarm(candidate_id);
            if (candidate == nullptr || candidate->id() == seeker.id()) {
                continue;
            }
            const auto candidate_lifecycle = swarm::evolution::evaluate(*candidate, config_.lifecycle);
            if (!candidate_lifecycle.alive || paired_this_block.count(candidate->id()) != 0) {
                continue;
            }

            candidates.push_back(candidate);
            const auto bankroll_response = swarm::social::request_private_info(seeker, *candidate, swarm::social::PrivateInfoField::bankroll, population_.rng(), config_.lifecycle);
            const auto readiness_response = swarm::social::request_private_info(seeker, *candidate, swarm::social::PrivateInfoField::reproductive_readiness, population_.rng(), config_.lifecycle);
            knowledge.push_back({
                swarm::social::make_public_face(*candidate),
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

        if (result.offspring->bankroll() > 0) {
            result.offspring->remove_bankroll(result.offspring->bankroll());
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
    std::int64_t starting_total = 0;
    for (std::size_t i = 0; i < assignment.swarm_ids.size(); ++i) {
        auto* swarm = population_.find_swarm(assignment.swarm_ids[i]);
        if (swarm == nullptr || !swarm->alive()) {
            continue;
        }
        table.set_player_name(i, "Swarm" + std::to_string(swarm->id()));
        table.set_player_stack_for_test(i, swarm->bankroll());
        starting_total += swarm->bankroll();
        seated.push_back(swarm);
    }
    if (seated.size() < 2) {
        return 0;
    }

    const auto chooser = [&](const swarm::poker::DecisionContext& context) {
        auto* swarm = seated.at(context.player_index);
        const auto state = make_state_vector(context);
        const auto decision = swarm->decide(population_.ocean(), state, static_cast<std::size_t>(context.hand_number));
        return map_decision_to_action(decision, context);
    };

    std::ostringstream sink;
    const auto summary = table.run(static_cast<int>(config_.hands_per_table_block), false, sink, chooser);

    std::int64_t ending_total = 0;
    for (std::size_t i = 0; i < seated.size() && i < summary.ending_stacks.size(); ++i) {
        auto* swarm = seated[i];
        const std::int64_t delta = summary.ending_stacks[i] - swarm->bankroll();
        if (delta > 0) {
            swarm->add_bankroll(delta);
        } else if (delta < 0) {
            swarm->remove_bankroll(-delta);
        }
        ending_total += swarm->bankroll();
        swarm->record_hands(static_cast<std::uint64_t>(summary.hands_played));
        auto& runtime = population_.runtime_state(swarm->id());
        runtime.total_hands_played_in_sim += static_cast<std::uint64_t>(summary.hands_played);
        runtime.tables_sat += 1;
    }

    const std::int64_t table_rake = summary.total_rake;
    if (starting_total != ending_total + table_rake) {
        throw std::logic_error("integrated table accounting drifted away from zero-sum plus rake burn");
    }
    if (table_rake > 0) {
        population_.chip_manager().burn_external(table_rake);
    }
    population_.chip_manager().require_invariants();
    return static_cast<std::uint64_t>(summary.hands_played) * static_cast<std::uint64_t>(seated.size());
}

} // namespace swarm::sim
