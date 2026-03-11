#include "swarm.h"

#include <algorithm>
#include <stdexcept>

namespace swarm::core {

Swarm::Swarm(Chromosome odd_chromosome, Chromosome even_chromosome)
    : odd_chromosome_(std::move(odd_chromosome)), even_chromosome_(std::move(even_chromosome)) {}

GovernanceMode Swarm::governance_for_turn(std::size_t hand_number) const noexcept {
    return (hand_number % 2 == 1) ? GovernanceMode::alpha_led : GovernanceMode::democratic;
}

Decision Swarm::decide(const Ocean& ocean, const PokerStateVector& state, std::size_t hand_number) const {
    if (odd_chromosome_.empty() || even_chromosome_.empty()) {
        throw std::invalid_argument("swarm requires both chromosomes");
    }

    const GovernanceMode mode = governance_for_turn(hand_number);
    if (mode == GovernanceMode::alpha_led) {
        return odd_chromosome_.agents().front().decide(ocean, state);
    }

    Decision odd = odd_chromosome_.decide(ocean, state);
    Decision even = even_chromosome_.decide(ocean, state);
    Decision merged;
    for (std::size_t i = 0; i < merged.action_scores.size(); ++i) {
        merged.action_scores[i] = 0.5f * (odd.action_scores[i] + even.action_scores[i]);
    }
    merged.raise_amount = 0.5f * (odd.raise_amount + even.raise_amount);
    merged.confidence = std::clamp(0.5f * (odd.confidence + even.confidence), 0.0f, 1.0f);

    std::size_t best_index = 0;
    for (std::size_t i = 1; i < merged.action_scores.size(); ++i) {
        if (merged.action_scores[i] > merged.action_scores[best_index]) {
            best_index = i;
        }
    }
    merged.action = static_cast<ActionType>(best_index);
    if (merged.action != ActionType::raise) {
        merged.raise_amount = 0.0f;
    }
    return merged;
}

const Chromosome& Swarm::odd_chromosome() const noexcept {
    return odd_chromosome_;
}

const Chromosome& Swarm::even_chromosome() const noexcept {
    return even_chromosome_;
}

} // namespace swarm::core
