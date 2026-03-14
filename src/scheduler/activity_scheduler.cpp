#include "activity_scheduler.h"

#include <algorithm>
#include <cmath>

namespace swarm::scheduler {

namespace {

float clamp_unit(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

std::uint32_t required_sleep_ticks(const ActivityState& state, const ActivityPreferences& preferences) {
    const auto cycle_ticks = std::max<std::uint32_t>(1, preferences.cycle_ticks);
    const auto ticks_used_in_cycle = static_cast<std::uint32_t>(state.total_ticks % cycle_ticks);
    const auto sleep_used_in_cycle = static_cast<std::uint32_t>(state.sleep_ticks % cycle_ticks);
    const auto target = static_cast<std::uint32_t>(std::ceil(static_cast<double>(cycle_ticks) * preferences.min_sleep_ratio));
    if (sleep_used_in_cycle >= target) {
        return 0;
    }
    return target - sleep_used_in_cycle;
}

} // namespace

ActivityScheduler::ActivityScheduler(ActivityPreferences defaults) : defaults_(defaults) {}

ActivityPreferences ActivityScheduler::defaults() const noexcept {
    return defaults_;
}

ActivityPreferences ActivityScheduler::derive_preferences(const swarm::core::Swarm& swarm) const noexcept {
    ActivityPreferences derived = defaults_;
    derived.play_bias = clamp_unit(0.25f + 0.45f * swarm.average_confidence_gene() + 0.30f * swarm.average_risk_gene());
    derived.rest_bias = clamp_unit(0.10f + 0.35f * swarm.average_honesty_gene() + 0.25f * swarm.average_skepticism_gene());
    derived.sleep_bias = std::max(derived.min_sleep_ratio, clamp_unit(1.0f - 0.55f * derived.play_bias + 0.15f * derived.rest_bias));
    return derived;
}

ActivityState ActivityScheduler::next_state(
    const swarm::core::Swarm& swarm,
    const swarm::evolution::LifecycleState& lifecycle,
    const TimePoint& time,
    const ActivityState* previous) const noexcept {
    ActivityState next = previous ? *previous : ActivityState{};
    next.preferences = previous ? previous->preferences : derive_preferences(swarm);
    next.mode = choose_mode(swarm, lifecycle, time, next);
    ++next.total_ticks;
    switch (next.mode) {
        case ActivityMode::play:
            ++next.play_ticks;
            break;
        case ActivityMode::active_rest:
            ++next.active_rest_ticks;
            break;
        case ActivityMode::sleep:
            ++next.sleep_ticks;
            break;
    }
    return next;
}

ActivityMode ActivityScheduler::choose_mode(
    const swarm::core::Swarm& swarm,
    const swarm::evolution::LifecycleState& lifecycle,
    const TimePoint& time,
    const ActivityState& baseline) const noexcept {
    const auto preferences = baseline.preferences;
    const auto cycle_ticks = std::max<std::uint32_t>(1, preferences.cycle_ticks);
    const auto tick_in_cycle = static_cast<std::uint32_t>(baseline.total_ticks % cycle_ticks);
    const auto remaining_slots = cycle_ticks - tick_in_cycle;
    const auto sleep_needed = required_sleep_ticks(baseline, preferences);

    if (!lifecycle.alive || swarm.bankroll() <= 0) {
        return ActivityMode::sleep;
    }

    if (sleep_needed >= remaining_slots) {
        return ActivityMode::sleep;
    }

    if (lifecycle.phase == swarm::evolution::LifePhase::old_age && sleep_needed > 0) {
        return ActivityMode::sleep;
    }

    const float phase_play_penalty = lifecycle.phase == swarm::evolution::LifePhase::youth ? 0.10f : 0.0f;
    const float phase_rest_bonus = lifecycle.phase == swarm::evolution::LifePhase::old_age ? 0.15f : 0.0f;
    const float reproduction_rest_bonus = lifecycle.reproduction_window_open ? 0.05f : 0.0f;
    const float time_progress = static_cast<float>(time.tick_in_block) / static_cast<float>(std::max<std::uint64_t>(1, time.tick_in_block + 1));

    const float play_score = preferences.play_bias - phase_play_penalty - 0.05f * time_progress;
    const float rest_score = preferences.rest_bias + phase_rest_bonus + reproduction_rest_bonus;
    const float sleep_score = preferences.sleep_bias + (sleep_needed > 0 ? 0.20f : 0.0f);

    if (sleep_score >= play_score && sleep_score >= rest_score) {
        return ActivityMode::sleep;
    }
    if (play_score >= rest_score) {
        return ActivityMode::play;
    }
    return ActivityMode::active_rest;
}

bool eligible_for_play(const swarm::core::Swarm& swarm, ActivityMode mode, const swarm::evolution::LifecycleState& lifecycle) noexcept {
    return mode == ActivityMode::play && lifecycle.alive && swarm.alive() && swarm.bankroll() > 0;
}

} // namespace swarm::scheduler
