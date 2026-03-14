#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "activity_scheduler.h"
#include "../core/swarm.h"
#include "../evolution/lifecycle.h"

namespace swarm::scheduler {

struct TableAssignment {
    std::size_t table_id = 0;
    std::vector<std::uint64_t> swarm_ids;
};

struct SchedulingPassEntry {
    const swarm::core::Swarm* swarm = nullptr;
    ActivityMode mode = ActivityMode::sleep;
    swarm::evolution::LifecycleState lifecycle{};
};

struct TablePlan {
    std::vector<TableAssignment> tables;
    std::vector<std::uint64_t> deferred_swarm_ids;
};

class TableManager {
public:
    static constexpr std::size_t target_table_size = 8;
    static constexpr std::size_t minimum_table_size = 2;

    [[nodiscard]] TablePlan build_plan(const std::vector<SchedulingPassEntry>& entries) const;
};

} // namespace swarm::scheduler
