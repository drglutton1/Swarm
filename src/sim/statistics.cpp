#include "statistics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "simulation.h"

namespace swarm::sim {
namespace {

double shannon_diversity(std::uint64_t fold_actions, std::uint64_t check_call_actions, std::uint64_t raise_actions) {
    const double total = static_cast<double>(fold_actions + check_call_actions + raise_actions);
    if (total <= 0.0) {
        return 0.0;
    }

    double entropy = 0.0;
    for (const auto count : {fold_actions, check_call_actions, raise_actions}) {
        if (count == 0) {
            continue;
        }
        const double p = static_cast<double>(count) / total;
        entropy -= p * std::log(p);
    }
    return entropy / std::log(3.0);
}

} // namespace

void StatisticsCollector::capture(const Population& population, const swarm::scheduler::TimePoint& time,
    std::size_t tables_formed, std::uint64_t hands_resolved, std::size_t offspring_born, std::size_t deaths_processed,
    const BlockTelemetry& telemetry) {
    SimulationSnapshot snapshot;
    snapshot.tick = time.tick;
    snapshot.hand_block = time.hand_block;
    snapshot.tables_formed = tables_formed;
    snapshot.hands_resolved = hands_resolved;
    snapshot.offspring_born = offspring_born;
    snapshot.reproduction_attempts = telemetry.reproduction_attempts;
    snapshot.reproduction_successes = telemetry.reproduction_successes;
    snapshot.deaths_processed = deaths_processed;
    snapshot.deaths_age = telemetry.deaths_age;
    snapshot.deaths_bankruptcy = telemetry.deaths_bankruptcy;
    snapshot.deaths_other = telemetry.deaths_other;
    snapshot.fold_actions = telemetry.fold_actions;
    snapshot.check_call_actions = telemetry.check_call_actions;
    snapshot.raise_actions = telemetry.raise_actions;
    snapshot.decision_diversity = shannon_diversity(telemetry.fold_actions, telemetry.check_call_actions, telemetry.raise_actions);
    snapshot.total_swarms = population.swarms().size() + population.dead_swarms().size();
    snapshot.dead_swarms = population.dead_swarms().size();
    snapshot.min_bankroll = population.swarms().empty() ? 0 : std::numeric_limits<std::int64_t>::max();
    snapshot.max_bankroll = population.swarms().empty() ? 0 : std::numeric_limits<std::int64_t>::min();
    snapshot.active_rest_cost = telemetry.active_rest_cost;
    snapshot.sleep_cost = telemetry.sleep_cost;

    for (const auto& swarm : population.swarms()) {
        const auto lifecycle = swarm::evolution::evaluate(swarm);
        snapshot.total_bankroll += swarm.bankroll();
        snapshot.min_bankroll = std::min(snapshot.min_bankroll, swarm.bankroll());
        snapshot.max_bankroll = std::max(snapshot.max_bankroll, swarm.bankroll());
        if (lifecycle.alive) {
            ++snapshot.alive_swarms;
        }
        switch (lifecycle.phase) {
            case swarm::evolution::LifePhase::youth:
                ++snapshot.youth_swarms;
                break;
            case swarm::evolution::LifePhase::maturity:
                ++snapshot.mature_swarms;
                break;
            case swarm::evolution::LifePhase::old_age:
                ++snapshot.old_age_swarms;
                break;
            case swarm::evolution::LifePhase::dead:
                break;
        }
        if (lifecycle.reproduction_window_open) {
            ++snapshot.reproduction_ready_swarms;
        }

        const auto& runtime = population.runtime_state(swarm.id());
        snapshot.cumulative_play_ticks += runtime.activity.play_ticks;
        snapshot.cumulative_rest_ticks += runtime.activity.active_rest_ticks;
        snapshot.cumulative_sleep_ticks += runtime.activity.sleep_ticks;
        switch (runtime.activity.mode) {
            case swarm::scheduler::ActivityMode::play:
                ++snapshot.playing_swarms;
                break;
            case swarm::scheduler::ActivityMode::active_rest:
                ++snapshot.resting_swarms;
                break;
            case swarm::scheduler::ActivityMode::sleep:
                ++snapshot.sleeping_swarms;
                break;
        }
    }

    const double total_active = static_cast<double>(snapshot.playing_swarms + snapshot.resting_swarms + snapshot.sleeping_swarms);
    if (total_active > 0.0) {
        snapshot.block_play_ratio = static_cast<double>(snapshot.playing_swarms) / total_active;
        snapshot.block_rest_ratio = static_cast<double>(snapshot.resting_swarms) / total_active;
        snapshot.block_sleep_ratio = static_cast<double>(snapshot.sleeping_swarms) / total_active;
    }

    snapshot.average_bankroll = snapshot.alive_swarms == 0 ? 0.0 : static_cast<double>(snapshot.total_bankroll) / static_cast<double>(snapshot.alive_swarms);
    snapshot.chip_accounting_ok = population.chip_manager().invariants_hold() && population.chip_manager().chips_in_play() == snapshot.total_bankroll;
    history_.push_back(snapshot);
}

const std::vector<SimulationSnapshot>& StatisticsCollector::history() const noexcept { return history_; }

const SimulationSnapshot& StatisticsCollector::latest() const {
    if (history_.empty()) {
        throw std::logic_error("no simulation statistics have been captured");
    }
    return history_.back();
}

std::string StatisticsCollector::to_json() const {
    std::ostringstream out;
    out << "{\n  \"history\": [\n";
    for (std::size_t i = 0; i < history_.size(); ++i) {
        const auto& s = history_[i];
        out << "    {\"tick\":" << s.tick
            << ",\"hand_block\":" << s.hand_block
            << ",\"total_swarms\":" << s.total_swarms
            << ",\"alive_swarms\":" << s.alive_swarms
            << ",\"dead_swarms\":" << s.dead_swarms
            << ",\"youth_swarms\":" << s.youth_swarms
            << ",\"mature_swarms\":" << s.mature_swarms
            << ",\"old_age_swarms\":" << s.old_age_swarms
            << ",\"reproduction_ready_swarms\":" << s.reproduction_ready_swarms
            << ",\"playing_swarms\":" << s.playing_swarms
            << ",\"resting_swarms\":" << s.resting_swarms
            << ",\"sleeping_swarms\":" << s.sleeping_swarms
            << ",\"block_play_ratio\":" << s.block_play_ratio
            << ",\"block_rest_ratio\":" << s.block_rest_ratio
            << ",\"block_sleep_ratio\":" << s.block_sleep_ratio
            << ",\"tables_formed\":" << s.tables_formed
            << ",\"offspring_born\":" << s.offspring_born
            << ",\"reproduction_attempts\":" << s.reproduction_attempts
            << ",\"reproduction_successes\":" << s.reproduction_successes
            << ",\"deaths_processed\":" << s.deaths_processed
            << ",\"deaths_age\":" << s.deaths_age
            << ",\"deaths_bankruptcy\":" << s.deaths_bankruptcy
            << ",\"deaths_other\":" << s.deaths_other
            << ",\"hands_resolved\":" << s.hands_resolved
            << ",\"fold_actions\":" << s.fold_actions
            << ",\"check_call_actions\":" << s.check_call_actions
            << ",\"raise_actions\":" << s.raise_actions
            << ",\"decision_diversity\":" << s.decision_diversity
            << ",\"total_bankroll\":" << s.total_bankroll
            << ",\"min_bankroll\":" << s.min_bankroll
            << ",\"max_bankroll\":" << s.max_bankroll
            << ",\"active_rest_cost\":" << s.active_rest_cost
            << ",\"sleep_cost\":" << s.sleep_cost
            << ",\"cumulative_play_ticks\":" << s.cumulative_play_ticks
            << ",\"cumulative_rest_ticks\":" << s.cumulative_rest_ticks
            << ",\"cumulative_sleep_ticks\":" << s.cumulative_sleep_ticks
            << ",\"average_bankroll\":" << s.average_bankroll
            << ",\"chip_accounting_ok\":" << (s.chip_accounting_ok ? "true" : "false") << '}';
        if (i + 1 != history_.size()) {
            out << ',';
        }
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

} // namespace swarm::sim
