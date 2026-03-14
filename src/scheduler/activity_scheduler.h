#pragma once

#include <cstdint>

#include "time_manager.h"
#include "../core/swarm.h"
#include "../evolution/lifecycle.h"

namespace swarm::scheduler {

enum class ActivityMode {
    play,
    active_rest,
    sleep,
};

struct ActivityPreferences {
    float play_bias = 0.5f;
    float rest_bias = 0.2f;
    float sleep_bias = 0.3f;
    std::uint32_t cycle_ticks = 10;
    float min_sleep_ratio = 0.30f;
};

struct ActivityState {
    ActivityMode mode = ActivityMode::sleep;
    std::uint64_t total_ticks = 0;
    std::uint64_t play_ticks = 0;
    std::uint64_t active_rest_ticks = 0;
    std::uint64_t sleep_ticks = 0;
    ActivityPreferences preferences{};
};

class ActivityScheduler {
public:
    explicit ActivityScheduler(ActivityPreferences defaults = {});

    [[nodiscard]] ActivityPreferences defaults() const noexcept;
    [[nodiscard]] ActivityPreferences derive_preferences(const swarm::core::Swarm& swarm) const noexcept;
    [[nodiscard]] ActivityState next_state(
        const swarm::core::Swarm& swarm,
        const swarm::evolution::LifecycleState& lifecycle,
        const TimePoint& time,
        const ActivityState* previous = nullptr) const noexcept;

private:
    ActivityPreferences defaults_;

    [[nodiscard]] ActivityMode choose_mode(
        const swarm::core::Swarm& swarm,
        const swarm::evolution::LifecycleState& lifecycle,
        const TimePoint& time,
        const ActivityState& baseline) const noexcept;
};

[[nodiscard]] bool eligible_for_play(const swarm::core::Swarm& swarm, ActivityMode mode, const swarm::evolution::LifecycleState& lifecycle) noexcept;

} // namespace swarm::scheduler
