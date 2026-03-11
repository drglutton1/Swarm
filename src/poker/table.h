#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
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
    std::int64_t street_commitment {0};
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
    Table(int player_count, std::int64_t starting_stack, std::int64_t small_blind, std::int64_t big_blind, std::int64_t rake_per_hand, std::uint64_t seed)
        : small_blind_(small_blind), big_blind_(big_blind), pot_manager_(rake_per_hand), rng_(seed) {
        players_.reserve(player_count);
        for (int i = 0; i < player_count; ++i) {
            players_.push_back(PlayerState{"Bot" + std::to_string(i + 1), starting_stack});
        }
        pot_manager_.prepare_players(players_.size());
        initial_total_chips_ = total_chips();
        button_index_ = players_.empty() ? 0 : players_.size() - 1;
    }

    HandLog play_hand(int hand_number, bool verbose) {
        HandLog log {hand_number, {}};
        if (funded_player_count() < 2) {
            if (verbose) {
                log.events.push_back("Not enough funded players to deal another hand");
            }
            return log;
        }

        deck_.reset();
        deck_.shuffle(rng_);
        board_.clear();
        pot_manager_.reset_hand();
        reset_hand_state();

        const std::size_t dealer = advance_button();
        const bool heads_up = funded_player_count() == 2;
        const std::size_t sb = heads_up ? dealer : next_funded_player(dealer);
        const std::size_t bb = next_funded_player(sb);

        if (verbose) {
            log.events.push_back("Dealer: " + players_[dealer].name);
        }

        post_blind(sb, small_blind_, "small blind", log, verbose);
        post_blind(bb, big_blind_, "big blind", log, verbose);

        deal_hole_cards(dealer, verbose, log);

        if (!betting_round(next_funded_player(bb), big_blind_, "Preflop", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        log_board("Flop", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, "Flop", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        log_board("Turn", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, "Turn", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        log_board("River", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, "River", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        return showdown(log, verbose);
    }

    SimulationSummary run(int hands, bool verbose, std::ostream& os) {
        const auto start = std::chrono::steady_clock::now();
        int actually_played = 0;
        for (int hand = 1; hand <= hands; ++hand) {
            if (funded_player_count() < 2) {
                break;
            }
            const HandLog log = play_hand(hand, verbose);
            ++actually_played;
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
        return {actually_played, static_cast<int>(players_.size()), initial_total_chips_, total_chips(), pot_manager_.total_rake(), elapsed_ms, current_stacks()};
    }

    [[nodiscard]] bool chip_conservation_holds() const noexcept {
        return total_chips() + pot_manager_.total_rake() == initial_total_chips_;
    }

private:
    void reset_hand_state() {
        for (auto& player : players_) {
            player.folded = player.stack <= 0;
            player.contribution = 0;
            player.street_commitment = 0;
        }
    }

    [[nodiscard]] std::size_t funded_player_count() const noexcept {
        std::size_t count = 0;
        for (const auto& player : players_) {
            if (player.stack > 0) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t active_player_count() const noexcept {
        std::size_t count = 0;
        for (const auto& player : players_) {
            if (!player.folded && player.contribution > 0) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t players_able_to_act() const noexcept {
        std::size_t count = 0;
        for (const auto& player : players_) {
            if (!player.folded && player.stack > 0) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::vector<std::size_t> active_players() const {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i < players_.size(); ++i) {
            if (!players_[i].folded && players_[i].contribution > 0) {
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

    [[nodiscard]] std::int64_t total_chips() const noexcept {
        std::int64_t total = 0;
        for (const auto& player : players_) {
            total += player.stack;
        }
        return total;
    }

    [[nodiscard]] std::size_t advance_button() {
        for (std::size_t step = 1; step <= players_.size(); ++step) {
            const std::size_t idx = (button_index_ + step) % players_.size();
            if (players_[idx].stack > 0) {
                button_index_ = idx;
                return idx;
            }
        }
        return button_index_;
    }

    [[nodiscard]] std::size_t next_funded_player(std::size_t from) const {
        for (std::size_t step = 1; step <= players_.size(); ++step) {
            const std::size_t idx = (from + step) % players_.size();
            if (players_[idx].stack > 0) {
                return idx;
            }
        }
        return from;
    }

    void collect_bet(std::size_t player_index, std::int64_t amount) {
        if (amount <= 0) {
            return;
        }
        auto& player = players_[player_index];
        player.stack -= amount;
        player.contribution += amount;
        player.street_commitment += amount;
        pot_manager_.collect(player_index, amount);
    }

    void post_blind(std::size_t player_index, std::int64_t amount, const std::string& blind_name, HandLog& log, bool verbose) {
        auto& player = players_[player_index];
        const std::int64_t paid = (player.stack < amount) ? player.stack : amount;
        collect_bet(player_index, paid);
        if (verbose) {
            std::ostringstream oss;
            oss << player.name << " posts " << blind_name << ' ' << paid << " (stack=" << player.stack << ')';
            log.events.push_back(oss.str());
        }
    }

    void deal_hole_cards(std::size_t dealer, bool verbose, HandLog& log) {
        for (int card_no = 0; card_no < 2; ++card_no) {
            std::size_t cursor = next_funded_player(dealer);
            for (std::size_t dealt = 0; dealt < funded_player_count(); ++dealt) {
                players_[cursor].hole_cards[card_no] = deck_.draw();
                cursor = next_funded_player(cursor);
            }
        }
        if (verbose) {
            std::size_t cursor = next_funded_player(dealer);
            for (std::size_t dealt = 0; dealt < funded_player_count(); ++dealt) {
                std::ostringstream oss;
                oss << players_[cursor].name << " dealt " << players_[cursor].hole_cards[0] << ' ' << players_[cursor].hole_cards[1];
                log.events.push_back(oss.str());
                cursor = next_funded_player(cursor);
            }
        }
    }

    bool betting_round(std::size_t start_index, std::int64_t current_bet, const std::string& street_name, HandLog& log, bool verbose) {
        const std::vector<std::int64_t> carry = current_street_commitments();
        for (auto& player : players_) {
            player.street_commitment = 0;
        }
        if (current_bet > 0) {
            restore_preflop_blinds(carry);
        }

        std::size_t cursor = start_index;
        std::size_t settled = 0;
        std::size_t acting_players = players_able_to_act();
        bool raise_used = false;

        while (acting_players > 1 && settled < acting_players) {
            auto& player = players_[cursor];
            if (!player.folded && player.stack > 0) {
                const std::int64_t to_call = current_bet > player.street_commitment ? current_bet - player.street_commitment : 0;
                if (to_call > 0 && player.stack < to_call) {
                    player.folded = true;
                    if (verbose) {
                        log.events.push_back(street_name + ": " + player.name + " folds facing bet");
                    }
                    if (active_player_count() <= 1) {
                        return false;
                    }
                } else {
                    const double roll = rng_.uniform_real();
                    if (to_call > 0 && roll < 0.30) {
                        player.folded = true;
                        if (verbose) {
                            log.events.push_back(street_name + ": " + player.name + " folds");
                        }
                        if (active_player_count() <= 1) {
                            return false;
                        }
                    } else if (!raise_used && player.stack > to_call + big_blind_ && roll > 0.84) {
                        const std::int64_t raise_to = current_bet + big_blind_;
                        const std::int64_t invest = raise_to - player.street_commitment;
                        collect_bet(cursor, invest);
                        current_bet = raise_to;
                        raise_used = true;
                        settled = 1;
                        if (verbose) {
                            std::ostringstream oss;
                            oss << street_name << ": " << player.name << " raises to " << raise_to;
                            log.events.push_back(oss.str());
                        }
                    } else {
                        collect_bet(cursor, to_call);
                        ++settled;
                        if (verbose) {
                            std::ostringstream oss;
                            oss << street_name << ": " << player.name;
                            if (to_call > 0) {
                                oss << " calls " << to_call;
                            } else {
                                oss << " checks";
                            }
                            log.events.push_back(oss.str());
                        }
                    }
                }
            }
            cursor = next_funded_player(cursor);
            acting_players = players_able_to_act();
        }

        return active_player_count() > 1;
    }

    [[nodiscard]] std::vector<std::int64_t> current_street_commitments() const {
        std::vector<std::int64_t> commitments;
        commitments.reserve(players_.size());
        for (const auto& player : players_) {
            commitments.push_back(player.street_commitment);
        }
        return commitments;
    }

    void restore_preflop_blinds(const std::vector<std::int64_t>& commitments) {
        for (std::size_t i = 0; i < players_.size(); ++i) {
            players_[i].street_commitment = commitments[i];
        }
    }

    void log_board(const std::string& street_name, HandLog& log, bool verbose) const {
        if (!verbose) {
            return;
        }
        std::ostringstream oss;
        oss << street_name << ":";
        for (const auto& card : board_) {
            oss << ' ' << card;
        }
        log.events.push_back(oss.str());
    }

    HandLog award_uncontested(HandLog log, bool verbose) {
        auto survivors = active_players();
        if (survivors.empty()) {
            for (std::size_t i = 0; i < players_.size(); ++i) {
                if (players_[i].stack > 0 || players_[i].contribution > 0) {
                    survivors.push_back(i);
                    break;
                }
            }
        }
        pot_manager_.take_rake();
        auto stacks = current_stacks();
        pot_manager_.award_even_split({survivors.front()}, stacks);
        apply_stacks(stacks);
        if (verbose) {
            log.events.push_back(players_[survivors.front()].name + " wins uncontested pot");
        }
        return log;
    }

    HandLog showdown(HandLog log, bool verbose) {
        HandValue best_value {};
        bool has_best = false;
        std::vector<std::size_t> winners;

        for (const std::size_t idx : active_players()) {
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
                if (i) {
                    oss << ", ";
                }
                oss << players_[winners[i]].name;
            }
            oss << " with " << describe_hand(best_value) << "; rake=" << pot_manager_.rake_this_hand();
            log.events.push_back(oss.str());
        }
        return log;
    }

    std::int64_t small_blind_ {0};
    std::int64_t big_blind_ {0};
    std::int64_t initial_total_chips_ {0};
    std::size_t button_index_ {0};
    Deck deck_;
    PotManager pot_manager_;
    swarm::util::Rng rng_;
    std::vector<PlayerState> players_;
    std::vector<Card> board_;
};

} // namespace swarm::poker
