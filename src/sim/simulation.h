#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "population.h"
#include "statistics.h"
#include "../evolution/lifecycle.h"
#include "../evolution/reproduction.h"
#include "../scheduler/activity_scheduler.h"
#include "../scheduler/table_manager.h"
#include "../scheduler/time_manager.h"

namespace swarm::sim {

struct SimulationConfig {
    PopulationConfig population{};
    swarm::scheduler::TimeConfig time{};
    swarm::scheduler::ActivityPreferences activity{};
    swarm::evolution::LifecyclePolicy lifecycle{};
    swarm::evolution::ReproductionPolicy reproduction{};
    std::uint64_t hands_per_table_block = 5;
    std::int64_t small_blind = 5;
    std::int64_t big_blind = 5;
    std::int64_t rake_per_hand = 0;
    std::int64_t ubi_amount = 150;
    std::uint64_t ubi_interval_blocks = 30;
    bool ubi_enabled = true;
};

struct BlockResult {
    swarm::scheduler::TimePoint time{};
    std::size_t tables_formed = 0;
    std::uint64_t hands_resolved = 0;
    std::size_t offspring_born = 0;
    std::size_t injected_births = 0;
    std::size_t deaths_processed = 0;
    std::int64_t active_rest_cost = 0;
    std::int64_t sleep_cost = 0;
    std::int64_t ubi_paid = 0;
    bool chip_accounting_ok = false;
};

struct BlockTelemetry {
    std::uint64_t fold_actions = 0;
    std::uint64_t check_call_actions = 0;
    std::uint64_t raise_actions = 0;
    std::size_t reproduction_attempts = 0;
    std::size_t reproduction_successes = 0;
    std::size_t births_reproduction = 0;
    std::size_t births_injection = 0;
    std::size_t deaths_age = 0;
    std::size_t deaths_bankruptcy = 0;
    std::size_t deaths_other = 0;
    std::size_t active_play = 0;
    std::size_t active_rest = 0;
    std::size_t active_sleep = 0;
    std::int64_t active_rest_cost = 0;
    std::int64_t sleep_cost = 0;
    std::int64_t ubi_paid = 0;
};

class Simulation {
public:
    explicit Simulation(SimulationConfig config = {});

    [[nodiscard]] const SimulationConfig& config() const noexcept;
    [[nodiscard]] Population& population() noexcept;
    [[nodiscard]] const Population& population() const noexcept;
    [[nodiscard]] swarm::scheduler::TimeManager& time() noexcept;
    [[nodiscard]] const swarm::scheduler::TimeManager& time() const noexcept;
    [[nodiscard]] StatisticsCollector& statistics() noexcept;
    [[nodiscard]] const StatisticsCollector& statistics() const noexcept;

    BlockResult step_block();
    void run_blocks(std::size_t block_count);
    [[nodiscard]] std::string latest_statistics_json() const;

private:
    SimulationConfig config_{};
    Population population_{};
    swarm::scheduler::TimeManager time_{};
    swarm::scheduler::ActivityScheduler activity_scheduler_{};
    swarm::scheduler::TableManager table_manager_{};
    StatisticsCollector statistics_{};

    void process_deaths(std::size_t& death_count, BlockTelemetry& telemetry);
    void apply_ubi(BlockTelemetry& telemetry);
    std::size_t attempt_social_reproduction(BlockTelemetry& telemetry);
    std::int64_t apply_activity_costs(const BlockTelemetry& telemetry, std::int64_t& active_rest_cost, std::int64_t& sleep_cost);
    std::uint64_t run_table_block(const swarm::scheduler::TableAssignment& assignment, std::uint64_t seed_offset, BlockTelemetry& telemetry);
};

} // namespace swarm::sim
