#include "swarm.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace swarm::core {

std::uint64_t Swarm::next_id_ = 1;

namespace {

float mean_gene(const Chromosome& first, const Chromosome& second, float Genome::*member) {
    return 0.5f * (first.blueprint().*member + second.blueprint().*member);
}

Decision finalize_decision(Decision decision) {
    float score_sum = 0.0f;
    for (float& score : decision.action_scores) {
        score = std::max(0.0f, score);
        score_sum += score;
    }

    if (score_sum <= 0.0f) {
        decision.action_scores = {0.0f, 1.0f, 0.0f};
    } else {
        for (float& score : decision.action_scores) {
            score /= score_sum;
        }
    }

    std::size_t best_index = 0;
    for (std::size_t i = 1; i < decision.action_scores.size(); ++i) {
        if (decision.action_scores[i] > decision.action_scores[best_index]) {
            best_index = i;
        }
    }
    decision.action = static_cast<ActionType>(best_index);
    decision.confidence = std::clamp(decision.confidence, 0.0f, 1.0f);
    if (decision.action != ActionType::raise) {
        decision.raise_amount = 0.0f;
    } else {
        decision.raise_amount = std::max(0.0f, decision.raise_amount);
    }
    return decision;
}

Decision blend_decisions(const Decision& first, float first_weight, const Decision& second, float second_weight) {
    Decision merged;
    const float total_weight = first_weight + second_weight;
    if (total_weight <= 0.0f) {
        return finalize_decision(merged);
    }

    for (std::size_t i = 0; i < merged.action_scores.size(); ++i) {
        merged.action_scores[i] = (first.action_scores[i] * first_weight + second.action_scores[i] * second_weight) / total_weight;
    }
    merged.raise_amount = (first.raise_amount * first_weight + second.raise_amount * second_weight) / total_weight;
    merged.confidence = (first.confidence * first_weight + second.confidence * second_weight) / total_weight;
    return finalize_decision(merged);
}

Decision merge_democratic(const Decision& first, const Decision& second) {
    return blend_decisions(first, 1.0f, second, 1.0f);
}

Decision aggregate_advisor_signal(const Chromosome& chromosome, const Ocean& ocean, const PokerStateVector& state) {
    if (chromosome.size() <= 1) {
        return {};
    }

    Decision aggregate;
    float total_weight = 0.0f;
    const auto& agents = chromosome.agents();
    for (std::size_t i = 1; i < agents.size(); ++i) {
        const Decision advisor = agents[i].decide(ocean, state);
        const float rank_weight = 1.0f + 0.15f * static_cast<float>(i - 1);
        const float confidence_weight = 0.35f + advisor.confidence;
        const float weight = rank_weight * confidence_weight;
        total_weight += weight;
        for (std::size_t score_index = 0; score_index < aggregate.action_scores.size(); ++score_index) {
            aggregate.action_scores[score_index] += advisor.action_scores[score_index] * weight;
        }
        aggregate.raise_amount += advisor.raise_amount * weight;
        aggregate.confidence += advisor.confidence * weight;
    }

    if (total_weight <= 0.0f) {
        return {};
    }

    for (float& score : aggregate.action_scores) {
        score /= total_weight;
    }
    aggregate.raise_amount /= total_weight;
    aggregate.confidence /= total_weight;
    return finalize_decision(aggregate);
}

Decision decide_alpha_led(const Chromosome& dominant, const Chromosome& advisor_chromosome, const Ocean& ocean, const PokerStateVector& state) {
    const Decision alpha = dominant.agents().front().decide(ocean, state);
    const Decision inner_advisors = aggregate_advisor_signal(dominant, ocean, state);
    const Decision external_advisors = advisor_chromosome.decide(ocean, state);

    const float alpha_bias = std::clamp(dominant.blueprint().alpha_bias, 0.0f, 1.0f);
    const float alpha_weight = 1.1f + alpha_bias * 1.9f + alpha.confidence * 0.5f;
    const float advisor_weight = 0.35f + (1.0f - alpha_bias) * 0.9f + inner_advisors.confidence * 0.45f + external_advisors.confidence * 0.35f;
    const Decision combined_advisors = blend_decisions(inner_advisors, 1.0f, external_advisors, 0.8f);
    return blend_decisions(alpha, alpha_weight, combined_advisors, advisor_weight);
}

} // namespace

Swarm::Swarm(Chromosome first_chromosome, Chromosome second_chromosome, std::int64_t bankroll)
    : first_chromosome_(std::move(first_chromosome)), second_chromosome_(std::move(second_chromosome)), id_(next_id_++), bankroll_(bankroll) {
    if (first_chromosome_.empty() || second_chromosome_.empty()) {
        throw std::invalid_argument("swarm requires both chromosomes");
    }
    if (total_agents() < 4 || total_agents() > 18) {
        throw std::invalid_argument("swarm must contain 4 to 18 total agents");
    }
    if (bankroll_ < 0) {
        throw std::invalid_argument("swarm bankroll cannot be negative");
    }
}

Swarm Swarm::random(std::uint32_t ocean_size, std::size_t state_size, swarm::util::Rng& rng) {
    const std::size_t first_size = static_cast<std::size_t>(rng.next_int(2, 9));
    const std::size_t second_size = static_cast<std::size_t>(rng.next_int(2, 9));
    const DecoderConfig config{state_size, 4, 3};

    Genome first_blueprint = Genome::random(state_size, ocean_size, rng, static_cast<std::uint32_t>(first_size));
    Genome second_blueprint = Genome::random(state_size, ocean_size, rng, static_cast<std::uint32_t>(second_size));

    return Swarm(
        Chromosome::spawn_from_blueprint(first_blueprint, first_size, config, ocean_size),
        Chromosome::spawn_from_blueprint(second_blueprint, second_size, config, ocean_size),
        starting_bankroll);
}

GovernanceMode Swarm::governance_mode() const noexcept {
    return (total_agents() % 2 == 1) ? GovernanceMode::alpha_led : GovernanceMode::democratic;
}

Decision Swarm::decide(const Ocean& ocean, const PokerStateVector& state, std::size_t) const {
    if (governance_mode() == GovernanceMode::alpha_led) {
        const bool first_is_dominant = first_chromosome_.size() >= second_chromosome_.size();
        const Chromosome& dominant = first_is_dominant ? first_chromosome_ : second_chromosome_;
        const Chromosome& advisors = first_is_dominant ? second_chromosome_ : first_chromosome_;
        return decide_alpha_led(dominant, advisors, ocean, state);
    }

    return merge_democratic(first_chromosome_.decide(ocean, state), second_chromosome_.decide(ocean, state));
}

const Chromosome& Swarm::first_chromosome() const noexcept {
    return first_chromosome_;
}

const Chromosome& Swarm::second_chromosome() const noexcept {
    return second_chromosome_;
}

std::uint64_t Swarm::id() const noexcept {
    return id_;
}

std::size_t Swarm::total_agents() const noexcept {
    return first_chromosome_.size() + second_chromosome_.size();
}

std::int64_t Swarm::bankroll() const noexcept {
    return bankroll_;
}

bool Swarm::alive() const noexcept {
    return alive_;
}

std::uint64_t Swarm::hands_played() const noexcept {
    return hands_played_;
}

std::uint64_t Swarm::last_reproduction_hand() const noexcept {
    return last_reproduction_hand_;
}

std::uint32_t Swarm::offspring_count() const noexcept {
    return offspring_count_;
}

float Swarm::average_risk_gene() const noexcept {
    return mean_gene(first_chromosome_, second_chromosome_, &Genome::risk_gene);
}

float Swarm::average_confidence_gene() const noexcept {
    return mean_gene(first_chromosome_, second_chromosome_, &Genome::confidence_gene);
}

float Swarm::average_honesty_gene() const noexcept {
    return mean_gene(first_chromosome_, second_chromosome_, &Genome::honesty_gene);
}

float Swarm::average_skepticism_gene() const noexcept {
    return mean_gene(first_chromosome_, second_chromosome_, &Genome::skepticism_gene);
}

void Swarm::add_bankroll(std::int64_t amount) {
    if (amount < 0) {
        throw std::invalid_argument("cannot add a negative bankroll amount");
    }
    bankroll_ += amount;
}

void Swarm::remove_bankroll(std::int64_t amount) {
    if (amount < 0) {
        throw std::invalid_argument("cannot remove a negative bankroll amount");
    }
    if (amount > bankroll_) {
        throw std::invalid_argument("cannot remove more bankroll than available");
    }
    bankroll_ -= amount;
}

void Swarm::record_hands(std::uint64_t count) noexcept {
    hands_played_ += count;
}

void Swarm::set_hands_played(std::uint64_t count) noexcept {
    hands_played_ = count;
}

void Swarm::note_reproduction() noexcept {
    last_reproduction_hand_ = hands_played_;
    ++offspring_count_;
}

void Swarm::mark_dead() noexcept {
    alive_ = false;
}

} // namespace swarm::core
