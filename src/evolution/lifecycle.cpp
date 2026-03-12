#include "lifecycle.h"

namespace swarm::evolution {

LifecycleState evaluate(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy) {
    LifecycleState state;
    state.alive = swarm.alive() && swarm.bankroll() > 0 && swarm.hands_played() < policy.death_hands;
    if (!state.alive) {
        state.phase = LifePhase::dead;
        state.reproduction_window_open = false;
        return state;
    }

    if (swarm.hands_played() < policy.maturity_start_hands) {
        state.phase = LifePhase::youth;
    } else if (swarm.hands_played() < policy.old_age_start_hands) {
        state.phase = LifePhase::maturity;
    } else {
        state.phase = LifePhase::old_age;
    }

    const bool cooldown_ok = swarm.hands_played() >= swarm.last_reproduction_hand() + policy.reproduction_cooldown_hands;
    const bool offspring_cap_ok = swarm.offspring_count() < policy.max_offspring;
    state.reproduction_window_open = state.phase == LifePhase::maturity && cooldown_ok && offspring_cap_ok;
    return state;
}

bool is_alive(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy) {
    return evaluate(swarm, policy).alive;
}

bool can_reproduce(const swarm::core::Swarm& swarm, const LifecyclePolicy& policy) {
    return evaluate(swarm, policy).reproduction_window_open;
}

void apply_mortality(swarm::core::Swarm& swarm, const LifecyclePolicy& policy) {
    if (!is_alive(swarm, policy)) {
        swarm.mark_dead();
    }
}

} // namespace swarm::evolution
