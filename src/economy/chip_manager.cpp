#include "chip_manager.h"

namespace swarm::economy {

std::int64_t ChipManager::chips_injected() const noexcept {
    return chips_injected_;
}

std::int64_t ChipManager::chips_burned() const noexcept {
    return chips_burned_;
}

std::int64_t ChipManager::chips_in_play() const noexcept {
    return chips_in_play_;
}

void ChipManager::inject(swarm::core::Swarm& swarm, std::int64_t amount) {
    if (amount <= 0) {
        throw std::invalid_argument("inject amount must be positive");
    }
    swarm.add_bankroll(amount);
    chips_injected_ += amount;
    chips_in_play_ += amount;
}

void ChipManager::burn(swarm::core::Swarm& swarm, std::int64_t amount) {
    if (amount <= 0) {
        throw std::invalid_argument("burn amount must be positive");
    }
    swarm.remove_bankroll(amount);
    chips_burned_ += amount;
    chips_in_play_ -= amount;
}

void ChipManager::burn_external(std::int64_t amount) {
    if (amount <= 0) {
        throw std::invalid_argument("external burn amount must be positive");
    }
    chips_burned_ += amount;
    chips_in_play_ -= amount;
}

bool ChipManager::invariants_hold() const noexcept {
    return chips_injected_ >= 0 && chips_burned_ >= 0 && chips_in_play_ >= 0 && chips_in_play_ + chips_burned_ == chips_injected_;
}

void ChipManager::require_invariants() const {
    if (!invariants_hold()) {
        throw std::logic_error("chip accounting invariant violated");
    }
}

} // namespace swarm::economy
