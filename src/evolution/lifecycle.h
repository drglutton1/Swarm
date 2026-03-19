#pragma once

#include <cstdint>

#include "../core/swarm.h"

namespace swarm::evolution {

enum class LifePhase {
    youth,
    maturity,
    old_age,
    dead,
};

struct LifecyclePolicy {
    std::uint64_t maturity_start_hands = 3000;
    std::uint64_t old_age_start_hands = 90000;
    std::uint64_t death_hands = 100000;
    std::uint64_t reproduction_cooldown_hands = 3000;
    std::uint32_t max_offspring = 12;
};

struct LifecycleState {
    LifePhase phase = LifePhase::youth;
    bool alive = true;
    bool reproduction_window_open = false;
};

[[nodiscard]] LifecycleState evaluate(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy = {});
[[nodiscard]] bool is_alive(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy = {});
[[nodiscard]] bool can_reproduce(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy = {});
void apply_mortality(swarm::core::Swarm& swarm, const LifecyclePolicy& policy = {});

} // namespace swarm::evolution
