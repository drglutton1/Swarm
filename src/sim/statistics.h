#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "population.h"
#include "../evolution/lifecycle.h"
#include "../scheduler/time_manager.h"

namespace swarm::sim {

struct BlockTelemetry;

struct Per100BlockSummary {
    std::uint64_t end_block = 0;
    std::uint64_t start_block = 0;
    std::uint64_t fold_actions = 0;
    std::uint64_t check_call_actions = 0;
    std::uint64_t raise_actions = 0;
    double fold_ratio = 0.0;
    double check_call_ratio = 0.0;
    double raise_ratio = 0.0;
    double avg_risk_gene_alive = 0.0;
    double avg_honesty_gene_alive = 0.0;
    double avg_skepticism_gene_alive = 0.0;
    double reproduction_success_rate = 0.0;
    double avg_lineage_depth = 0.0;
    std::uint32_t max_lineage_depth = 0;
    std::vector<std::pair<std::uint64_t, std::size_t>> top_dynasties_living_descendants;
};

struct SimulationSnapshot {
    std::uint64_t tick = 0;
    std::uint64_t hand_block = 0;
    std::size_t total_swarms = 0;
    std::size_t alive_count = 0;
    std::size_t alive_swarms = 0;
    std::size_t dead_swarms = 0;
    std::size_t youth_swarms = 0;
    std::size_t mature_swarms = 0;
    std::size_t old_age_swarms = 0;
    std::size_t reproduction_ready_swarms = 0;
    std::size_t playing_swarms = 0;
    std::size_t resting_swarms = 0;
    std::size_t sleeping_swarms = 0;
    std::size_t tables_active = 0;
    std::size_t tables_formed = 0;
    std::size_t births_this_block_reproduction = 0;
    std::size_t births_this_block_injection = 0;
    std::size_t offspring_born = 0;
    std::size_t injected_births = 0;
    std::size_t reproduction_attempts = 0;
    std::size_t reproduction_successes = 0;
    std::size_t deaths_processed = 0;
    std::size_t deaths_this_block_bankruptcy = 0;
    std::size_t deaths_this_block_age = 0;
    std::size_t deaths_this_block_other = 0;
    std::size_t deaths_age = 0;
    std::size_t deaths_bankruptcy = 0;
    std::size_t deaths_other = 0;
    std::uint64_t hands_resolved = 0;
    std::uint64_t fold_actions = 0;
    std::uint64_t check_call_actions = 0;
    std::uint64_t raise_actions = 0;
    double decision_diversity = 0.0;
    std::int64_t total_chips_in_play = 0;
    std::int64_t total_chips_injected = 0;
    std::int64_t total_chips_burned = 0;
    std::int64_t total_bankroll = 0;
    std::int64_t min_bankroll = 0;
    std::int64_t max_bankroll = 0;
    std::int64_t active_rest_cost = 0;
    std::int64_t sleep_cost = 0;
    std::int64_t ubi_paid = 0;
    std::uint64_t cumulative_play_ticks = 0;
    std::uint64_t cumulative_rest_ticks = 0;
    std::uint64_t cumulative_sleep_ticks = 0;
    double avg_bankroll_alive = 0.0;
    double average_bankroll = 0.0;
    double median_bankroll_alive = 0.0;
    double block_play_ratio = 0.0;
    double block_rest_ratio = 0.0;
    double block_sleep_ratio = 0.0;
    double parity_ratio = 0.0;
    double avg_risk_gene_alive = 0.0;
    double avg_honesty_gene_alive = 0.0;
    double avg_skepticism_gene_alive = 0.0;
    double avg_lineage_depth_alive = 0.0;
    std::uint32_t max_lineage_depth_alive = 0;
    bool chip_accounting_ok = false;
};

class StatisticsCollector {
public:
    void capture(const Population& population, const swarm::scheduler::TimePoint& time, std::size_t tables_formed,
        std::uint64_t hands_resolved, std::size_t deaths_processed, const BlockTelemetry& telemetry);

    [[nodiscard]] const std::vector<SimulationSnapshot>& history() const noexcept;
    [[nodiscard]] const std::vector<Per100BlockSummary>& hundred_block_summaries() const noexcept;
    [[nodiscard]] const SimulationSnapshot& latest() const;
    [[nodiscard]] std::string to_json() const;

private:
    void capture_hundred_block_summary(const Population& population);

    std::vector<SimulationSnapshot> history_{};
    std::vector<Per100BlockSummary> hundred_block_summaries_{};
};

} // namespace swarm::sim
