#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "../core/ocean.h"
#include "../core/swarm.h"
#include "../economy/chip_manager.h"
#include "../scheduler/activity_scheduler.h"
#include "../util/rng.h"

namespace swarm::sim {

struct PopulationConfig {
    std::size_t swarm_count = 12;
    std::size_t ocean_size = 512;
    std::size_t state_size = 6;
    std::uint64_t seed = 1337;
    std::int64_t starting_bankroll = swarm::core::Swarm::starting_bankroll;
};

struct SwarmRuntimeState {
    swarm::scheduler::ActivityState activity{};
    std::uint64_t total_hands_played_in_sim = 0;
    std::uint64_t tables_sat = 0;
    std::uint64_t offspring_born_in_sim = 0;
    std::uint64_t action_counts[3] = {0, 0, 0};
};

class Population {
public:
    struct LineageInfo {
        std::uint64_t root_id = 0;
        std::uint64_t parent_a = 0;
        std::uint64_t parent_b = 0;
        std::uint32_t depth = 0;
    };

    Population() = default;
    explicit Population(PopulationConfig config);

    [[nodiscard]] static Population create_default(PopulationConfig config = {});

    [[nodiscard]] const PopulationConfig& config() const noexcept;
    [[nodiscard]] swarm::core::Ocean& ocean() noexcept;
    [[nodiscard]] const swarm::core::Ocean& ocean() const noexcept;
    [[nodiscard]] std::vector<swarm::core::Swarm>& swarms() noexcept;
    [[nodiscard]] const std::vector<swarm::core::Swarm>& swarms() const noexcept;
    [[nodiscard]] const std::vector<swarm::core::Swarm>& dead_swarms() const noexcept;
    [[nodiscard]] swarm::economy::ChipManager& chip_manager() noexcept;
    [[nodiscard]] const swarm::economy::ChipManager& chip_manager() const noexcept;
    [[nodiscard]] swarm::util::Rng& rng() noexcept;
    [[nodiscard]] const swarm::util::Rng& rng() const noexcept;
    [[nodiscard]] std::unordered_map<std::uint64_t, SwarmRuntimeState>& runtime() noexcept;
    [[nodiscard]] const std::unordered_map<std::uint64_t, SwarmRuntimeState>& runtime() const noexcept;

    [[nodiscard]] swarm::core::Swarm* find_swarm(std::uint64_t swarm_id) noexcept;
    [[nodiscard]] const swarm::core::Swarm* find_swarm(std::uint64_t swarm_id) const noexcept;
    [[nodiscard]] SwarmRuntimeState& runtime_state(std::uint64_t swarm_id);
    [[nodiscard]] const SwarmRuntimeState& runtime_state(std::uint64_t swarm_id) const;

    void register_offspring(std::uint64_t parent_a, std::uint64_t parent_b, std::uint64_t child_id);
    [[nodiscard]] std::vector<swarm::core::Swarm*> living_offspring_of(std::uint64_t parent_id);
    [[nodiscard]] swarm::core::Swarm spawn_random_swarm(bool seed_lifecycle = false);
    swarm::core::Swarm& append_swarm(swarm::core::Swarm swarm, std::int64_t bankroll);
    [[nodiscard]] const LineageInfo& lineage_info(std::uint64_t swarm_id) const;
    void refresh_ocean();
    void rebuild_index();
    void prune_dead();

private:
    void register_founder(std::uint64_t swarm_id);

    PopulationConfig config_{};
    swarm::core::Ocean ocean_{};
    std::vector<swarm::core::Swarm> swarms_{};
    std::vector<swarm::core::Swarm> dead_swarms_{};
    swarm::economy::ChipManager chip_manager_{};
    swarm::util::Rng rng_{0};
    std::unordered_map<std::uint64_t, SwarmRuntimeState> runtime_{};
    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> offspring_links_{};
    std::unordered_map<std::uint64_t, std::size_t> id_to_index_{};
    std::unordered_map<std::uint64_t, LineageInfo> lineage_{};
};

} // namespace swarm::sim
