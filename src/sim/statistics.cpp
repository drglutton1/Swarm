#include "statistics.h"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace swarm::sim {

void StatisticsCollector::capture(const Population& population, const swarm::scheduler::TimePoint& time,
    std::size_t tables_formed, std::uint64_t hands_resolved, std::size_t offspring_born, std::size_t deaths_processed) {
    SimulationSnapshot snapshot;
    snapshot.tick = time.tick;
    snapshot.hand_block = time.hand_block;
    snapshot.tables_formed = tables_formed;
    snapshot.hands_resolved = hands_resolved;
    snapshot.offspring_born = offspring_born;
    snapshot.deaths_processed = deaths_processed;
    snapshot.total_swarms = population.swarms().size() + population.dead_swarms().size();
    snapshot.dead_swarms = population.dead_swarms().size();
    snapshot.min_bankroll = population.swarms().empty() ? 0 : std::numeric_limits<std::int64_t>::max();
    snapshot.max_bankroll = population.swarms().empty() ? 0 : std::numeric_limits<std::int64_t>::min();

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
            << ",\"tables_formed\":" << s.tables_formed
            << ",\"offspring_born\":" << s.offspring_born
            << ",\"deaths_processed\":" << s.deaths_processed
            << ",\"hands_resolved\":" << s.hands_resolved
            << ",\"total_bankroll\":" << s.total_bankroll
            << ",\"min_bankroll\":" << s.min_bankroll
            << ",\"max_bankroll\":" << s.max_bankroll
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
