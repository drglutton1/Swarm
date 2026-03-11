#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "card.h"
#include "deck.h"
#include "hand_evaluator.h"
#include "pot_manager.h"
#include "../util/rng.h"

namespace swarm::poker {

struct PlayerState {
    std::string name;
    std::int64_t stack {0};
    std::array<Card, 2> hole_cards {};
    bool folded {false};
    std::int64_t contribution {0};
};

struct HandLog {
    int hand_number {0};
    std::vector<std::string> events;
};

struct SimulationSummary {
    int hands_played {0};
    int players {0};
    std::int64_t initial_chips {0};
    std::int64_t final_chips {0};
    std::int64_t total_rake {0};
    double elapsed_ms {0.0};
    std::vector<std::int64_t> ending_stacks;
};

class Table {
public:
    Table(int player_count, std::int64_t starting_stack, std::int64_t ante, std::int64_t rake_per_hand, std::uint64_t seed)
        : ante_(ante), pot_manager_(rake_per_hand), rng_(seed) {
        players_.reserve(player_count);
        for (int i = 0; i < player_count; ++i) {
            players_.push_back(PlayerState{"Bot" + std::to_string(i + 1), starting_stack});
        }
        pot_manager_.prepare_players(players_.size());
        initial_total_chips_ = total_chips();
    }

    HandLog play_hand(int hand_number, bool verbose) {
        deck_.reset();
        deck_.shuffle(rng_);
        board_.clear();
        pot_manager_.reset_hand();
        HandLog log {hand_number, {}};

        for (auto& player : players_) {
            player.folded = false;
            player.contribution = 0;
        }

        for (std::size_t i = 0; i < players_.size(); ++i) {
            auto& player = players_[i];
            const std::int64_t paid = std::min(player.stack, ante_);
            player.stack -= paid;
            player.contribution += paid;
            pot_manager_.collect(i, paid);
            if (verbose) {
                std::ostringstream oss;
                oss << player.name << " antes " << paid << " (stack=" << player.stack << ")";
                log.events.push_back(oss.str());
            }
        }

        for (auto& player : players_) {
            player.hole_cards = {deck_.draw(), deck_.draw()};
            if (verbose) {
                std::ostringstream oss;
                oss << player.name << " dealt " << player.hole_cards[0] << ' ' << player.hole_cards[1];
                log.events.push_back(oss.str());
            }
        }

        for (std::size_t i = 0; i < players_.size(); ++i) {
            auto& player = players_[i];
            const bool fold = rng_.uniform_real() < 0.35;
            player.folded = fold;
            if (verbose) {
                log.events.push_back(player.name + (fold ? " folds preflop" : " continues"));
            }
        }

        auto survivors = active_players();
        if (survivors.empty()) {
            std::size_t rescue = static_cast<std::size_t>(rng_.uniform_int<int>(0, static_cast<int>(players_.size() - 1)));
            players_[rescue].folded = false;
            survivors.push_back(rescue);
            if (verbose) {
                log.events.push_back(players_[rescue].name + " restored to avoid degenerate all-fold hand");
            }
        }

        if (survivors.size() == 1) {
            pot_manager_.take_rake();
            std::vector<std::int64_t> stacks = current_stacks();
            pot_manager_.award_even_split(survivors, stacks);
            apply_stacks(stacks);
            if (verbose) {
                log.events.push_back(players_[survivors.front()].name + " wins uncontested pot");
            }
            return log;
        }

        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());

        if (verbose) {
            std::ostringstream oss;
            oss << "Board: ";
            for (std::size_t i = 0; i < board_.size(); ++i) {
                if (i) oss << ' ';
                oss << board_[i];
            }
            log.events.push_back(oss.str());
        }

        HandValue best_value {};
        bool has_best = false;
        std::vector<std::size_t> winners;

        for (std::size_t idx : survivors) {
            std::vector<Card> cards;
            cards.reserve(7);
            cards.push_back(players_[idx].hole_cards[0]);
            cards.push_back(players_[idx].hole_cards[1]);
            cards.insert(cards.end(), board_.begin(), board_.end());
            const HandValue value = evaluate_best_of_seven(cards);
            if (verbose) {
                log.events.push_back(players_[idx].name + " shows " + describe_hand(value));
            }
            if (!has_best || best_value < value) {
                best_value = value;
                winners = {idx};
                has_best = true;
            } else if (value == best_value) {
                winners.push_back(idx);
            }
        }

        pot_manager_.take_rake();
        auto stacks = current_stacks();
        pot_manager_.award_even_split(winners, stacks);
        apply_stacks(stacks);

        if (verbose) {
            std::ostringstream oss;
            oss << "Winner" << (winners.size() > 1 ? "s" : "") << ": ";
            for (std::size_t i = 0; i < winners.size(); ++i) {
                if (i) oss << ", ";
                oss << players_[winners[i]].name;
            }
            oss << " with " << describe_hand(best_value) << "; rake=" << pot_manager_.rake_this_hand();
            log.events.push_back(oss.str());
        }

        return log;
    }

    SimulationSummary run(int hands, bool verbose, std::ostream& os) {
        const auto start = std::chrono::steady_clock::now();
        for (int hand = 1; hand <= hands; ++hand) {
            const HandLog log = play_hand(hand, verbose);
            if (verbose) {
                os << "=== Hand " << log.hand_number << " ===\n";
                for (const auto& event : log.events) {
                    os << "  " << event << '\n';
                }
                os << "  Stacks:";
                for (const auto& player : players_) {
                    os << ' ' << player.name << '=' << player.stack;
                }
                os << "\n\n";
            }
        }
        const auto end = std::chrono::steady_clock::now();
        const double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return {hands, static_cast<int>(players_.size()), initial_total_chips_, total_chips(), pot_manager_.total_rake(), elapsed_ms, current_stacks()};
    }

    [[nodiscard]] bool chip_conservation_holds() const noexcept {
        return total_chips() + pot_manager_.total_rake() == initial_total_chips_;
    }

private:
    [[nodiscard]] std::vector<std::size_t> active_players() const {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i < players_.size(); ++i) {
            if (!players_[i].folded) {
                result.push_back(i);
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<std::int64_t> current_stacks() const {
        std::vector<std::int64_t> out;
        out.reserve(players_.size());
        for (const auto& player : players_) {
            out.push_back(player.stack);
        }
        return out;
    }

    void apply_stacks(const std::vector<std::int64_t>& stacks) {
        for (std::size_t i = 0; i < players_.size(); ++i) {
            players_[i].stack = stacks[i];
        }
    }

    [[nodiscard]] std::int64_t total_chips() const {
        std::int64_t total = 0;
        for (const auto& player : players_) {
            total += player.stack;
        }
        return total;
    }

    std::int64_t ante_ {0};
    std::int64_t initial_total_chips_ {0};
    Deck deck_;
    PotManager pot_manager_;
    swarm::util::Rng rng_;
    std::vector<PlayerState> players_;
    std::vector<Card> board_;
};

} // namespace swarm::poker
