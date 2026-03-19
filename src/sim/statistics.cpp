#include "statistics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
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

double median_of_sorted(const std::vector<std::int64_t>& values) {
    if (values.empty()) {
        return 0.0;
    }
    const auto mid = values.size() / 2;
    if ((values.size() % 2U) == 1U) {
        return static_cast<double>(values[mid]);
    }
    return 0.5 * static_cast<double>(values[mid - 1] + values[mid]);
}

} // namespace

void StatisticsCollector::capture(const Population& population, const swarm::scheduler::TimePoint& time,
    std::size_t tables_formed, std::uint64_t hands_resolved, std::size_t deaths_processed, const BlockTelemetry& telemetry) {
    SimulationSnapshot snapshot;
    snapshot.tick = time.tick;
    snapshot.hand_block = time.hand_block;
    snapshot.tables_active = tables_formed;
    snapshot.tables_formed = tables_formed;
    snapshot.hands_resolved = hands_resolved;
    snapshot.births_this_block_reproduction = telemetry.births_reproduction;
    snapshot.births_this_block_injection = telemetry.births_injection;
    snapshot.offspring_born = telemetry.births_reproduction;
    snapshot.injected_births = telemetry.births_injection;
    snapshot.reproduction_attempts = telemetry.reproduction_attempts;
    snapshot.reproduction_successes = telemetry.reproduction_successes;
    snapshot.deaths_processed = deaths_processed;
    snapshot.deaths_this_block_age = telemetry.deaths_age;
    snapshot.deaths_this_block_bankruptcy = telemetry.deaths_bankruptcy;
    snapshot.deaths_this_block_other = telemetry.deaths_other;
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
    snapshot.ubi_paid = telemetry.ubi_paid;
    snapshot.total_chips_injected = population.chip_manager().chips_injected();
    snapshot.total_chips_burned = population.chip_manager().chips_burned();
    snapshot.total_chips_in_play = population.chip_manager().chips_in_play();

    std::vector<std::int64_t> bankrolls;
    bankrolls.reserve(population.swarms().size());
    std::size_t even_alive = 0;
    std::size_t odd_alive = 0;
    std::map<std::uint64_t, std::size_t> dynasty_sizes;

    for (const auto& swarm : population.swarms()) {
        const auto lifecycle = swarm::evolution::evaluate(swarm);
        snapshot.total_bankroll += swarm.bankroll();
        snapshot.min_bankroll = std::min(snapshot.min_bankroll, swarm.bankroll());
        snapshot.max_bankroll = std::max(snapshot.max_bankroll, swarm.bankroll());
        bankrolls.push_back(swarm.bankroll());

        if (lifecycle.alive) {
            ++snapshot.alive_count;
            ++snapshot.alive_swarms;
            snapshot.avg_risk_gene_alive += swarm.average_risk_gene();
            snapshot.avg_honesty_gene_alive += swarm.average_honesty_gene();
            snapshot.avg_skepticism_gene_alive += swarm.average_skepticism_gene();
            const auto& lineage = population.lineage_info(swarm.id());
            snapshot.avg_lineage_depth_alive += lineage.depth;
            snapshot.max_lineage_depth_alive = std::max(snapshot.max_lineage_depth_alive, lineage.depth);
            dynasty_sizes[lineage.root_id] += 1;
            if ((swarm.total_agents() % 2U) == 0U) {
                ++even_alive;
            } else {
                ++odd_alive;
            }
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

    std::sort(bankrolls.begin(), bankrolls.end());
    snapshot.avg_bankroll_alive = snapshot.alive_count == 0 ? 0.0 : static_cast<double>(snapshot.total_bankroll) / static_cast<double>(snapshot.alive_count);
    snapshot.average_bankroll = snapshot.avg_bankroll_alive;
    snapshot.median_bankroll_alive = median_of_sorted(bankrolls);
    snapshot.parity_ratio = (odd_alive == 0) ? static_cast<double>(even_alive) : static_cast<double>(even_alive) / static_cast<double>(odd_alive);
    if (snapshot.alive_count > 0) {
        snapshot.avg_risk_gene_alive /= static_cast<double>(snapshot.alive_count);
        snapshot.avg_honesty_gene_alive /= static_cast<double>(snapshot.alive_count);
        snapshot.avg_skepticism_gene_alive /= static_cast<double>(snapshot.alive_count);
        snapshot.avg_lineage_depth_alive /= static_cast<double>(snapshot.alive_count);
    }
    snapshot.chip_accounting_ok = population.chip_manager().invariants_hold() && population.chip_manager().chips_in_play() == snapshot.total_chips_in_play;
    history_.push_back(snapshot);

    if ((snapshot.hand_block % 100U) == 0U) {
        capture_hundred_block_summary(population);
    }
}

const std::vector<SimulationSnapshot>& StatisticsCollector::history() const noexcept { return history_; }
const std::vector<Per100BlockSummary>& StatisticsCollector::hundred_block_summaries() const noexcept { return hundred_block_summaries_; }

const SimulationSnapshot& StatisticsCollector::latest() const {
    if (history_.empty()) {
        throw std::logic_error("no simulation statistics have been captured");
    }
    return history_.back();
}

void StatisticsCollector::capture_hundred_block_summary(const Population& population) {
    if (history_.empty()) {
        return;
    }

    const auto end_index = history_.size();
    const auto start_index = (end_index >= 100U) ? (end_index - 100U) : 0U;

    Per100BlockSummary summary;
    summary.start_block = history_[start_index].hand_block;
    summary.end_block = history_.back().hand_block;

    std::uint64_t reproduction_attempts = 0;
    std::uint64_t reproduction_successes = 0;
    for (std::size_t i = start_index; i < end_index; ++i) {
        summary.fold_actions += history_[i].fold_actions;
        summary.check_call_actions += history_[i].check_call_actions;
        summary.raise_actions += history_[i].raise_actions;
        reproduction_attempts += history_[i].reproduction_attempts;
        reproduction_successes += history_[i].reproduction_successes;
    }

    const double total_actions = static_cast<double>(summary.fold_actions + summary.check_call_actions + summary.raise_actions);
    if (total_actions > 0.0) {
        summary.fold_ratio = static_cast<double>(summary.fold_actions) / total_actions;
        summary.check_call_ratio = static_cast<double>(summary.check_call_actions) / total_actions;
        summary.raise_ratio = static_cast<double>(summary.raise_actions) / total_actions;
    }
    summary.reproduction_success_rate = reproduction_attempts == 0 ? 0.0 : static_cast<double>(reproduction_successes) / static_cast<double>(reproduction_attempts);

    std::map<std::uint64_t, std::size_t> dynasty_sizes;
    for (const auto& swarm : population.swarms()) {
        summary.avg_risk_gene_alive += swarm.average_risk_gene();
        summary.avg_honesty_gene_alive += swarm.average_honesty_gene();
        summary.avg_skepticism_gene_alive += swarm.average_skepticism_gene();
        const auto& lineage = population.lineage_info(swarm.id());
        summary.avg_lineage_depth += lineage.depth;
        summary.max_lineage_depth = std::max(summary.max_lineage_depth, lineage.depth);
        dynasty_sizes[lineage.root_id] += 1;
    }
    if (!population.swarms().empty()) {
        const auto alive = static_cast<double>(population.swarms().size());
        summary.avg_risk_gene_alive /= alive;
        summary.avg_honesty_gene_alive /= alive;
        summary.avg_skepticism_gene_alive /= alive;
        summary.avg_lineage_depth /= alive;
    }

    std::vector<std::pair<std::uint64_t, std::size_t>> dynasties(dynasty_sizes.begin(), dynasty_sizes.end());
    std::sort(dynasties.begin(), dynasties.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });
    if (dynasties.size() > 5U) {
        dynasties.resize(5U);
    }
    summary.top_dynasties_living_descendants = std::move(dynasties);
    hundred_block_summaries_.push_back(std::move(summary));
}

std::string StatisticsCollector::to_json() const {
    std::ostringstream out;
    out << "{\n  \"history\": [\n";
    for (std::size_t i = 0; i < history_.size(); ++i) {
        const auto& s = history_[i];
        out << "    {\"tick\":" << s.tick
            << ",\"hand_block\":" << s.hand_block
            << ",\"total_swarms\":" << s.total_swarms
            << ",\"alive_count\":" << s.alive_count
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
            << ",\"tables_active\":" << s.tables_active
            << ",\"tables_formed\":" << s.tables_formed
            << ",\"births_this_block_reproduction\":" << s.births_this_block_reproduction
            << ",\"births_this_block_injection\":" << s.births_this_block_injection
            << ",\"offspring_born\":" << s.offspring_born
            << ",\"injected_births\":" << s.injected_births
            << ",\"reproduction_attempts\":" << s.reproduction_attempts
            << ",\"reproduction_successes\":" << s.reproduction_successes
            << ",\"deaths_processed\":" << s.deaths_processed
            << ",\"deaths_this_block_age\":" << s.deaths_this_block_age
            << ",\"deaths_this_block_bankruptcy\":" << s.deaths_this_block_bankruptcy
            << ",\"deaths_this_block_other\":" << s.deaths_this_block_other
            << ",\"deaths_age\":" << s.deaths_age
            << ",\"deaths_bankruptcy\":" << s.deaths_bankruptcy
            << ",\"deaths_other\":" << s.deaths_other
            << ",\"hands_resolved\":" << s.hands_resolved
            << ",\"fold_actions\":" << s.fold_actions
            << ",\"check_call_actions\":" << s.check_call_actions
            << ",\"raise_actions\":" << s.raise_actions
            << ",\"decision_diversity\":" << s.decision_diversity
            << ",\"total_chips_in_play\":" << s.total_chips_in_play
            << ",\"total_chips_injected\":" << s.total_chips_injected
            << ",\"total_chips_burned\":" << s.total_chips_burned
            << ",\"total_bankroll\":" << s.total_bankroll
            << ",\"min_bankroll\":" << s.min_bankroll
            << ",\"max_bankroll\":" << s.max_bankroll
            << ",\"active_rest_cost\":" << s.active_rest_cost
            << ",\"sleep_cost\":" << s.sleep_cost
            << ",\"ubi_paid\":" << s.ubi_paid
            << ",\"cumulative_play_ticks\":" << s.cumulative_play_ticks
            << ",\"cumulative_rest_ticks\":" << s.cumulative_rest_ticks
            << ",\"cumulative_sleep_ticks\":" << s.cumulative_sleep_ticks
            << ",\"avg_bankroll_alive\":" << s.avg_bankroll_alive
            << ",\"average_bankroll\":" << s.average_bankroll
            << ",\"median_bankroll_alive\":" << s.median_bankroll_alive
            << ",\"parity_ratio\":" << s.parity_ratio
            << ",\"avg_risk_gene_alive\":" << s.avg_risk_gene_alive
            << ",\"avg_honesty_gene_alive\":" << s.avg_honesty_gene_alive
            << ",\"avg_skepticism_gene_alive\":" << s.avg_skepticism_gene_alive
            << ",\"avg_lineage_depth_alive\":" << s.avg_lineage_depth_alive
            << ",\"max_lineage_depth_alive\":" << s.max_lineage_depth_alive
            << ",\"chip_accounting_ok\":" << (s.chip_accounting_ok ? "true" : "false") << '}';
        if (i + 1 != history_.size()) {
            out << ',';
        }
        out << "\n";
    }
    out << "  ],\n  \"per_100_blocks\": [\n";
    for (std::size_t i = 0; i < hundred_block_summaries_.size(); ++i) {
        const auto& s = hundred_block_summaries_[i];
        out << "    {\"start_block\":" << s.start_block
            << ",\"end_block\":" << s.end_block
            << ",\"action_distribution\":{\"fold\":" << s.fold_ratio << ",\"check_call\":" << s.check_call_ratio << ",\"raise\":" << s.raise_ratio << '}'
            << ",\"avg_risk_gene_alive\":" << s.avg_risk_gene_alive
            << ",\"avg_honesty_gene_alive\":" << s.avg_honesty_gene_alive
            << ",\"avg_skepticism_gene_alive\":" << s.avg_skepticism_gene_alive
            << ",\"reproduction_success_rate\":" << s.reproduction_success_rate
            << ",\"avg_lineage_depth\":" << s.avg_lineage_depth
            << ",\"max_lineage_depth\":" << s.max_lineage_depth
            << ",\"top_dynasties_living_descendants\":[";
        for (std::size_t j = 0; j < s.top_dynasties_living_descendants.size(); ++j) {
            const auto& dynasty = s.top_dynasties_living_descendants[j];
            out << "{\"root_id\":" << dynasty.first << ",\"living_descendants\":" << dynasty.second << '}';
            if (j + 1 != s.top_dynasties_living_descendants.size()) {
                out << ',';
            }
        }
        out << "]}";
        if (i + 1 != hundred_block_summaries_.size()) {
            out << ',';
        }
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

} // namespace swarm::sim
