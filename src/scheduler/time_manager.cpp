#include "time_manager.h"

#include <stdexcept>

namespace swarm::scheduler {

TimeManager::TimeManager(TimeConfig config) : config_(config) {
    if (config_.hands_per_block == 0) {
        throw std::invalid_argument("hands_per_block must be positive");
    }
    if (config_.ticks_per_block == 0) {
        throw std::invalid_argument("ticks_per_block must be positive");
    }
}

const TimeConfig& TimeManager::config() const noexcept {
    return config_;
}

const TimePoint& TimeManager::now() const noexcept {
    return now_;
}

TimePoint TimeManager::advance_tick(std::uint64_t tick_count) {
    if (tick_count == 0) {
        return now_;
    }

    now_.tick += tick_count;
    now_.hand_block += (now_.tick_in_block + tick_count) / config_.ticks_per_block;
    now_.tick_in_block = (now_.tick_in_block + tick_count) % config_.ticks_per_block;
    now_.hands_elapsed = now_.hand_block * config_.hands_per_block;
    return now_;
}

TimePoint TimeManager::advance_hand_blocks(std::uint64_t block_count) {
    if (block_count == 0) {
        return now_;
    }

    now_.hand_block += block_count;
    now_.hands_elapsed += block_count * config_.hands_per_block;
    now_.tick = now_.hand_block * config_.ticks_per_block + now_.tick_in_block;
    return now_;
}

void TimeManager::reset() noexcept {
    now_ = {};
}

} // namespace swarm::scheduler
