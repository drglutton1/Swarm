#include "table_manager.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace swarm::scheduler {

TablePlan TableManager::build_plan(const std::vector<SchedulingPassEntry>& entries) const {
    std::vector<const swarm::core::Swarm*> eligible;
    eligible.reserve(entries.size());

    for (const auto& entry : entries) {
        if (entry.swarm == nullptr) {
            continue;
        }
        if (eligible_for_play(*entry.swarm, entry.mode, entry.lifecycle)) {
            eligible.push_back(entry.swarm);
        }
    }

    std::sort(eligible.begin(), eligible.end(), [](const auto* left, const auto* right) {
        if (left->bankroll() != right->bankroll()) {
            return left->bankroll() > right->bankroll();
        }
        return left->id() < right->id();
    });

    TablePlan plan;
    std::unordered_set<std::uint64_t> assigned;
    std::size_t cursor = 0;
    std::size_t table_id = 1;

    while (cursor + target_table_size <= eligible.size()) {
        TableAssignment table;
        table.table_id = table_id++;
        for (std::size_t i = 0; i < target_table_size; ++i) {
            const auto* swarm = eligible[cursor++];
            if (!assigned.insert(swarm->id()).second) {
                throw std::logic_error("swarm double-booked within scheduling pass");
            }
            table.swarm_ids.push_back(swarm->id());
        }
        plan.tables.push_back(std::move(table));
    }

    const std::size_t leftovers = eligible.size() - cursor;
    if (leftovers >= minimum_table_size) {
        TableAssignment table;
        table.table_id = table_id;
        for (std::size_t i = 0; i < leftovers; ++i) {
            const auto* swarm = eligible[cursor++];
            if (!assigned.insert(swarm->id()).second) {
                throw std::logic_error("swarm double-booked within leftovers policy");
            }
            table.swarm_ids.push_back(swarm->id());
        }
        plan.tables.push_back(std::move(table));
    } else {
        while (cursor < eligible.size()) {
            plan.deferred_swarm_ids.push_back(eligible[cursor++]->id());
        }
    }

    return plan;
}

} // namespace swarm::scheduler
