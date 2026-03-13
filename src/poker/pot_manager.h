#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace swarm::poker {

class PotManager {
public:
    explicit PotManager(std::int64_t rake_per_hand = 0) : rake_per_hand_(rake_per_hand) {}

    void reset_hand() {
        pot_ = 0;
        rake_taken_this_hand_ = 0;
        contributions_.assign(contributions_.size(), 0);
    }

    void prepare_players(std::size_t player_count) {
        contributions_.assign(player_count, 0);
        reset_hand();
    }

    void collect(std::size_t player_index, std::int64_t amount) {
        if (player_index >= contributions_.size()) {
            throw std::out_of_range("invalid player index");
        }
        if (amount < 0) {
            throw std::invalid_argument("cannot collect a negative amount");
        }
        contributions_[player_index] += amount;
        pot_ += amount;
    }

    std::int64_t take_rake(std::int64_t amount) {
        if (amount < 0) {
            throw std::invalid_argument("rake increment cannot be negative");
        }
        const std::int64_t rake = std::min(amount, pot_);
        pot_ -= rake;
        rake_taken_this_hand_ += rake;
        total_rake_ += rake;
        return rake;
    }

    std::int64_t take_rake() {
        return take_rake(rake_per_hand_);
    }

    [[nodiscard]] std::int64_t pot() const noexcept { return pot_; }
    [[nodiscard]] std::int64_t total_rake() const noexcept { return total_rake_; }
    [[nodiscard]] std::int64_t rake_this_hand() const noexcept { return rake_taken_this_hand_; }
    [[nodiscard]] const std::vector<std::int64_t>& contributions() const noexcept { return contributions_; }

    void award_even_split_amount(std::int64_t amount, const std::vector<std::size_t>& winners, std::vector<std::int64_t>& stacks) {
        if (winners.empty()) {
            throw std::invalid_argument("cannot award empty winner set");
        }
        if (amount < 0 || amount > pot_) {
            throw std::invalid_argument("invalid award amount");
        }
        const std::int64_t share = amount / static_cast<std::int64_t>(winners.size());
        const std::int64_t remainder = amount % static_cast<std::int64_t>(winners.size());
        for (std::size_t i = 0; i < winners.size(); ++i) {
            stacks[winners[i]] += share + (static_cast<std::int64_t>(i) < remainder ? 1 : 0);
        }
        pot_ -= amount;
    }

    void award_even_split(const std::vector<std::size_t>& winners, std::vector<std::int64_t>& stacks) {
        award_even_split_amount(pot_, winners, stacks);
    }

private:
    std::int64_t rake_per_hand_ {0};
    std::int64_t pot_ {0};
    std::int64_t total_rake_ {0};
    std::int64_t rake_taken_this_hand_ {0};
    std::vector<std::int64_t> contributions_;
};

} // namespace swarm::poker
