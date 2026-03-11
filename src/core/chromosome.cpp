#include "chromosome.h"

#include <algorithm>
#include <stdexcept>

namespace swarm::core {

Chromosome::Chromosome(std::vector<Agent> agents, Genome blueprint)
    : agents_(std::move(agents)), blueprint_(std::move(blueprint)) {}

Chromosome Chromosome::spawn_from_blueprint(const Genome& blueprint, std::size_t agent_count, const DecoderConfig& config, std::uint32_t ocean_size) {
    std::vector<Agent> agents;
    agents.reserve(agent_count);
    for (std::size_t slot = 0; slot < agent_count; ++slot) {
        agents.emplace_back(blueprint.derive_agent_variant(slot, ocean_size), config);
    }
    return Chromosome(std::move(agents), blueprint);
}

const std::vector<Agent>& Chromosome::agents() const noexcept {
    return agents_;
}

const Genome& Chromosome::blueprint() const noexcept {
    return blueprint_;
}

bool Chromosome::empty() const noexcept {
    return agents_.empty();
}

std::size_t Chromosome::size() const noexcept {
    return agents_.size();
}

Decision Chromosome::decide(const Ocean& ocean, const PokerStateVector& state) const {
    if (agents_.empty()) {
        throw std::invalid_argument("chromosome requires at least one agent");
    }

    Decision aggregate;
    float total_weight = 0.0f;
    for (std::size_t i = 0; i < agents_.size(); ++i) {
        const Decision decision = agents_[i].decide(ocean, state);
        const float weight = 1.0f + static_cast<float>(i == 0 ? blueprint_.alpha_bias : 0.0f);
        total_weight += weight;
        for (std::size_t score_index = 0; score_index < aggregate.action_scores.size(); ++score_index) {
            aggregate.action_scores[score_index] += decision.action_scores[score_index] * weight;
        }
        aggregate.raise_amount += decision.raise_amount * weight;
        aggregate.confidence += decision.confidence * weight;
    }

    const float scale = total_weight > 0.0f ? (1.0f / total_weight) : 1.0f;
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
