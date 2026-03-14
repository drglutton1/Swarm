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
    const auto sleep_used_in_cycle = static_cast<std::uint32_t>(state.sleep_ticks % cycle_ticks);
    const auto target = static_cast<std::uint32_t>(std::ceil(static_cast<double>(cycle_ticks) * preferences.min_sleep_ratio));
    if (sleep_used_in_cycle >= target) {
        return 0;
    }
    return target - sleep_used_in_cycle;
}

std::uint32_t target_rest_ticks(const swarm::evolution::LifecycleState& lifecycle, const ActivityPreferences& preferences) {
    const auto cycle_ticks = std::max<std::uint32_t>(1, preferences.cycle_ticks);
    const auto sleep_target = static_cast<std::uint32_t>(std::ceil(static_cast<double>(cycle_ticks) * preferences.min_sleep_ratio));
    float rest_ratio = 0.12f + 0.18f * preferences.rest_bias;
    if (lifecycle.phase == swarm::evolution::LifePhase::old_age) {
        rest_ratio += 0.08f;
    }
    if (lifecycle.reproduction_window_open) {
        rest_ratio += 0.04f;
    }
    auto target = static_cast<std::uint32_t>(std::round(static_cast<double>(cycle_ticks) * rest_ratio));
    target = std::clamp<std::uint32_t>(target, 1, cycle_ticks - sleep_target - 1);
    return target;
}

std::uint32_t required_rest_ticks(const ActivityState& state, const swarm::evolution::LifecycleState& lifecycle, const ActivityPreferences& preferences) {
    const auto cycle_ticks = std::max<std::uint32_t>(1, preferences.cycle_ticks);
    const auto used = static_cast<std::uint32_t>(state.active_rest_ticks % cycle_ticks);
    const auto target = target_rest_ticks(lifecycle, preferences);
    if (used >= target) {
        return 0;
    }
    return target - used;
}

} // namespace

ActivityScheduler::ActivityScheduler(ActivityPreferences defaults) : defaults_(defaults) {}

ActivityPreferences ActivityScheduler::defaults() const noexcept {
    return defaults_;
}

ActivityPreferences ActivityScheduler::derive_preferences(const swarm::core::Swarm& swarm) const noexcept {
    ActivityPreferences derived = defaults_;
    derived.play_bias = clamp_unit(0.38f + 0.42f * swarm.average_confidence_gene() + 0.28f * swarm.average_risk_gene());
    derived.rest_bias = clamp_unit(0.16f + 0.28f * swarm.average_honesty_gene() + 0.18f * swarm.average_skepticism_gene());
    derived.sleep_bias = clamp_unit(0.12f + 0.18f * (1.0f - swarm.average_confidence_gene()) + 0.12f * swarm.average_skepticism_gene());
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
    const auto rest_needed = required_rest_ticks(baseline, lifecycle, preferences);

    if (!lifecycle.alive || swarm.bankroll() <= 0) {
        return ActivityMode::sleep;
    }

    if (sleep_needed >= remaining_slots) {
        return ActivityMode::sleep;
    }
    if (rest_needed >= remaining_slots && sleep_needed == 0) {
        return ActivityMode::active_rest;
    }

    if (lifecycle.phase == swarm::evolution::LifePhase::old_age && sleep_needed > 0) {
        return ActivityMode::sleep;
    }

    const float phase_play_penalty = lifecycle.phase == swarm::evolution::LifePhase::youth ? 0.08f : 0.0f;
    const float phase_rest_bonus = lifecycle.phase == swarm::evolution::LifePhase::old_age ? 0.12f : 0.0f;
    const float reproduction_play_bonus = lifecycle.reproduction_window_open ? 0.05f : 0.0f;
    const float reproduction_rest_bonus = lifecycle.reproduction_window_open ? 0.03f : 0.0f;
    const float time_progress = static_cast<float>(time.tick_in_block) / static_cast<float>(std::max<std::uint32_t>(1, cycle_ticks - 1));
    const float block_opening_play_bonus = time.tick_in_block < 3 ? 0.08f : 0.0f;
    const float recent_play_penalty = baseline.mode == ActivityMode::play ? 0.08f : 0.0f;
    const float recent_sleep_penalty = baseline.mode == ActivityMode::sleep ? 0.03f : 0.0f;
    const float recent_rest_bonus = baseline.mode == ActivityMode::play ? 0.05f : 0.0f;
    const float sleep_pressure = sleep_needed > 0 ? 0.10f * static_cast<float>(sleep_needed) : 0.0f;
    const float rest_pressure = rest_needed > 0 ? 0.12f * static_cast<float>(rest_needed) : 0.0f;

    const float play_score = preferences.play_bias + reproduction_play_bonus + block_opening_play_bonus - phase_play_penalty - recent_play_penalty - 0.03f * time_progress;
    const float rest_score = preferences.rest_bias + phase_rest_bonus + reproduction_rest_bonus + recent_rest_bonus + rest_pressure;
    const float sleep_score = preferences.sleep_bias + sleep_pressure + recent_sleep_penalty;

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
