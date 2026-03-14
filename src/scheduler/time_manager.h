#pragma once

#include <cstddef>
#include <cstdint>

namespace swarm::scheduler {

struct TimeConfig {
    std::uint64_t hands_per_block = 100;
    std::uint64_t ticks_per_block = 10;
};

struct TimePoint {
    std::uint64_t tick = 0;
    std::uint64_t hand_block = 0;
    std::uint64_t hands_elapsed = 0;
    std::uint64_t tick_in_block = 0;
};

class TimeManager {
public:
    explicit TimeManager(TimeConfig config = {});

    [[nodiscard]] const TimeConfig& config() const noexcept;
    [[nodiscard]] const TimePoint& now() const noexcept;

    TimePoint advance_tick(std::uint64_t tick_count = 1);
    TimePoint advance_hand_blocks(std::uint64_t block_count = 1);
    void reset() noexcept;

private:
    TimeConfig config_;
    TimePoint now_;
};

} // namespace swarm::scheduler
