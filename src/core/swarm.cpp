#include "swarm.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace swarm::core {
namespace {

std::size_t pick_first_chromosome_size(std::size_t total_agents, swarm::util::Rng& rng) {
    std::vector<std::size_t> valid;
    for (std::size_t first = 2; first <= 9; ++first) {
        const auto second = total_agents - first;
        if (second >= 2 && second <= 9) {
            valid.push_back(first);
        }
    }
    if (valid.empty()) {
        throw std::invalid_argument("no valid chromosome split for swarm");
    }
    return valid[static_cast<std::size_t>(rng.uniform_int(0, static_cast<int>(valid.size() - 1)))];
}

Decision merge_democratic(const Decision& first, const Decision& second) {
    Decision merged;
    for (std::size_t i = 0; i < merged.action_scores.size(); ++i) {
        merged.action_scores[i] = 0.5f * (first.action_scores[i] + second.action_scores[i]);
    }
    merged.raise_amount = 0.5f * (first.raise_amount + second.raise_amount);
    merged.confidence = std::clamp(0.5f * (first.confidence + second.confidence), 0.0f, 1.0f);

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

} // namespace

Swarm::Swarm(Chromosome first_chromosome, Chromosome second_chromosome, std::int64_t bankroll)
    : first_chromosome_(std::move(first_chromosome)), second_chromosome_(std::move(second_chromosome)), bankroll_(bankroll) {
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
    const std::size_t total_agents = static_cast<std::size_t>(rng.uniform_int(4, 18));
    const std::size_t first_size = pick_first_chromosome_size(total_agents, rng);
    const std::size_t second_size = total_agents - first_size;
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
        const Chromosome& dominant = first_chromosome_.size() >= second_chromosome_.size() ? first_chromosome_ : second_chromosome_;
        return dominant.agents().front().decide(ocean, state);
    }

    return merge_democratic(first_chromosome_.decide(ocean, state), second_chromosome_.decide(ocean, state));
}

const Chromosome& Swarm::first_chromosome() const noexcept {
    return first_chromosome_;
}

const Chromosome& Swarm::second_chromosome() const noexcept {
    return second_chromosome_;
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
