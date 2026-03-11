#include "chromosome.h"

#include <algorithm>
#include <stdexcept>

namespace swarm::core {

Chromosome::Chromosome(std::vector<Agent> agents) : agents_(std::move(agents)) {}

const std::vector<Agent>& Chromosome::agents() const noexcept {
    return agents_;
}

bool Chromosome::empty() const noexcept {
    return agents_.empty();
}

Decision Chromosome::decide(const Ocean& ocean, const PokerStateVector& state) const {
    if (agents_.empty()) {
        throw std::invalid_argument("chromosome requires at least one agent");
    }

    Decision aggregate;
    for (const Agent& agent : agents_) {
        const Decision decision = agent.decide(ocean, state);
        for (std::size_t i = 0; i < aggregate.action_scores.size(); ++i) {
            aggregate.action_scores[i] += decision.action_scores[i];
        }
        aggregate.raise_amount += decision.raise_amount;
        aggregate.confidence += decision.confidence;
    }

    const float scale = 1.0f / static_cast<float>(agents_.size());
    for (float& score : aggregate.action_scores) {
        score *= scale;
    }
    aggregate.raise_amount *= scale;
    aggregate.confidence = std::clamp(aggregate.confidence * scale, 0.0f, 1.0f);

    std::size_t best_index = 0;
    for (std::size_t i = 1; i < aggregate.action_scores.size(); ++i) {
        if (aggregate.action_scores[i] > aggregate.action_scores[best_index]) {
            best_index = i;
        }
    }
    aggregate.action = static_cast<ActionType>(best_index);
    if (aggregate.action != ActionType::raise) {
        aggregate.raise_amount = 0.0f;
    }

    return aggregate;
}

} // namespace swarm::core
