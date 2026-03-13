#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
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

struct ForcedAction {
    enum class Type {
        fold,
        check_call,
        raise_to,
    };

    Type type {Type::check_call};
    std::int64_t amount {0};
};

struct ForcedRoundResult {
    bool hand_continues {true};
    std::int64_t current_bet {0};
    std::int64_t min_raise_to {0};
    std::vector<PlayerState> players;
    std::vector<std::string> events;
};

class Table {
public:
    Table(int player_count, std::int64_t starting_stack, std::int64_t small_blind, std::int64_t big_blind, std::int64_t rake_per_hand, std::uint64_t seed)
        : small_blind_(small_blind), big_blind_(big_blind), flop_rake_(rake_per_hand), pot_manager_(rake_per_hand), rng_(seed) {
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

        if (!betting_round(next_funded_player(bb), big_blind_, big_blind_, "Preflop", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        board_.push_back(deck_.draw());
        apply_street_rake(Street::flop);
        log_board("Flop", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, big_blind_, "Flop", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        apply_street_rake(Street::turn);
        log_board("Turn", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, big_blind_, "Turn", log, verbose)) {
            return award_uncontested(log, verbose);
        }

        board_.push_back(deck_.draw());
        apply_street_rake(Street::river);
        log_board("River", log, verbose);
        if (!betting_round(next_funded_player(dealer), 0, big_blind_, "River", log, verbose)) {
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

    void set_player_stack_for_test(std::size_t player_index, std::int64_t stack) {
        players_.at(player_index).stack = stack;
        initial_total_chips_ = total_chips();
    }

    void set_button_for_test(std::size_t button_index) {
        button_index_ = button_index % players_.size();
    }

    ForcedRoundResult run_forced_betting_round_for_test(
        std::vector<std::int64_t> stacks,
        std::vector<std::int64_t> street_commitments,
        std::vector<bool> folded,
        std::size_t start_index,
        std::int64_t current_bet,
        std::int64_t min_raise,
        const std::string& street_name,
        const std::vector<ForcedAction>& script,
        bool verbose = true) {

        if (stacks.size() != players_.size() || street_commitments.size() != players_.size() || folded.size() != players_.size()) {
            throw std::invalid_argument("forced betting round vectors must match player count");
        }

        auto saved_players = players_;
        auto saved_pot = pot_manager_;

        pot_manager_.reset_hand();
        for (std::size_t i = 0; i < players_.size(); ++i) {
            players_[i].stack = stacks[i];
            players_[i].street_commitment = street_commitments[i];
            players_[i].contribution = street_commitments[i];
            players_[i].folded = folded[i];
            if (street_commitments[i] > 0) {
                pot_manager_.collect(i, street_commitments[i]);
            }
        }

        std::size_t script_index = 0;
        HandLog log {0, {}};
        const auto chooser = [&](std::size_t, std::int64_t, std::int64_t, std::int64_t) {
            if (script_index >= script.size()) {
                return ForcedAction{};
            }
            return script[script_index++];
        };

        const bool hand_continues = betting_round_impl(start_index, current_bet, min_raise, street_name, log, verbose, chooser);
        ForcedRoundResult result;
        result.hand_continues = hand_continues;
        result.current_bet = current_bet_;
        result.min_raise_to = min_raise_to_;
        result.players = players_;
        result.events = std::move(log.events);

        players_ = std::move(saved_players);
        pot_manager_ = std::move(saved_pot);
        current_bet_ = 0;
        min_raise_to_ = 0;
        return result;
    }

    std::vector<std::int64_t> resolve_showdown_for_test(
        std::vector<std::int64_t> stacks,
        const std::vector<std::int64_t>& contributions,
        const std::vector<bool>& folded,
        const std::vector<HandValue>& hand_values,
        std::int64_t rake_taken = 0) {

        if (stacks.size() != players_.size() || contributions.size() != players_.size() || folded.size() != players_.size() || hand_values.size() != players_.size()) {
            throw std::invalid_argument("showdown test vectors must match player count");
        }

        auto saved_players = players_;
        auto saved_pot = pot_manager_;

        pot_manager_.reset_hand();
        for (std::size_t i = 0; i < players_.size(); ++i) {
            players_[i].stack = stacks[i];
            players_[i].contribution = contributions[i];
            players_[i].street_commitment = 0;
            players_[i].folded = folded[i];
            if (contributions[i] > 0) {
                pot_manager_.collect(i, contributions[i]);
            }
        }
        if (rake_taken > 0) {
            pot_manager_.take_rake(rake_taken);
        }

        distribute_showdown_pots(stacks, contributions, folded, hand_values);

        players_ = std::move(saved_players);
        pot_manager_ = std::move(saved_pot);
        return stacks;
    }

private:
    enum class Street {
        preflop,
        flop,
        turn,
        river,
    };

    void reset_hand_state() {
        current_bet_ = 0;
        min_raise_to_ = 0;
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
            if (!player.folded) {
                ++count;
            }
        }
        return count;
    }

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

    [[nodiscard]] std::size_t next_player_to_act(std::size_t from) const {
        for (std::size_t step = 1; step <= players_.size(); ++step) {
            const std::size_t idx = (from + step) % players_.size();
            if (!players_[idx].folded && players_[idx].stack > 0) {
                return idx;
            }
        }
        return from;
    }

    [[nodiscard]] bool player_can_act(std::size_t index) const noexcept {
        return !players_[index].folded && players_[index].stack > 0;
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

    template <typename ActionChooser>
    bool betting_round_impl(
        std::size_t start_index,
        std::int64_t current_bet,
        std::int64_t min_raise,
        const std::string& street_name,
        HandLog& log,
        bool verbose,
        ActionChooser chooser) {

        current_bet_ = current_bet;
        min_raise_to_ = current_bet + std::max<std::int64_t>(min_raise, big_blind_);

        std::vector<bool> needs_action(players_.size(), false);
        for (std::size_t i = 0; i < players_.size(); ++i) {
            needs_action[i] = player_can_act(i);
        }

        if (active_player_count() <= 1) {
            return false;
        }

        std::size_t cursor = start_index;
        while (any_needs_action(needs_action)) {
            if (!player_can_act(cursor) || !needs_action[cursor]) {
                cursor = next_player_to_act(cursor);
                continue;
            }

            auto& player = players_[cursor];
            const std::int64_t to_call = std::max<std::int64_t>(0, current_bet_ - player.street_commitment);
            ForcedAction action = chooser(cursor, to_call, current_bet_, min_raise_to_);
            const auto before_commitment = player.street_commitment;
            const auto before_bet = current_bet_;

            if (action.type == ForcedAction::Type::fold && to_call == 0) {
                action.type = ForcedAction::Type::check_call;
            }

            if (action.type == ForcedAction::Type::fold) {
                player.folded = true;
                needs_action[cursor] = false;
                if (verbose) {
                    log.events.push_back(street_name + ": " + player.name + " folds");
                }
                if (active_player_count() <= 1) {
                    return false;
                }
            } else if (action.type == ForcedAction::Type::check_call) {
                const std::int64_t invest = std::min(player.stack, to_call);
                collect_bet(cursor, invest);
                needs_action[cursor] = false;
                if (verbose) {
                    std::ostringstream oss;
                    oss << street_name << ": " << player.name;
                    if (to_call <= 0) {
                        oss << " checks";
                    } else if (invest < to_call) {
                        oss << " calls all-in " << invest;
                    } else {
                        oss << " calls " << invest;
                    }
                    log.events.push_back(oss.str());
                }
            } else {
                std::int64_t requested_raise_to = std::max(action.amount, current_bet_);
                std::int64_t max_raise_to = player.street_commitment + player.stack;
                requested_raise_to = std::min(requested_raise_to, max_raise_to);
                if (requested_raise_to <= current_bet_) {
                    const std::int64_t invest = std::min(player.stack, to_call);
                    collect_bet(cursor, invest);
                    needs_action[cursor] = false;
                    if (verbose) {
                        std::ostringstream oss;
                        oss << street_name << ": " << player.name;
                        if (invest < to_call) {
                            oss << " calls all-in " << invest;
                        } else if (to_call > 0) {
                            oss << " calls " << invest;
                        } else {
                            oss << " checks";
                        }
                        log.events.push_back(oss.str());
                    }
                } else {
                    const std::int64_t invest = requested_raise_to - player.street_commitment;
                    collect_bet(cursor, invest);
                    const std::int64_t actual_raise_to = player.street_commitment;
                    const std::int64_t raise_size = actual_raise_to - before_bet;
                    current_bet_ = actual_raise_to;
                    const bool full_raise = raise_size >= std::max<std::int64_t>(min_raise_to_ - before_bet, big_blind_);
                    if (full_raise) {
                        min_raise_to_ = current_bet_ + raise_size;
                        reopen_action(needs_action, cursor);
                    } else {
                        needs_action[cursor] = false;
                    }
                    if (verbose) {
                        std::ostringstream oss;
                        oss << street_name << ": " << player.name;
                        if (player.stack == 0) {
                            if (full_raise) {
                                oss << " raises all-in to " << actual_raise_to;
                            } else {
                                oss << " calls all-in " << invest;
                            }
                        } else {
                            oss << " raises to " << actual_raise_to;
                        }
                        log.events.push_back(oss.str());
                    }
                }
            }

            if (player_can_act(cursor) && player.street_commitment == before_commitment && before_bet == current_bet_) {
                needs_action[cursor] = false;
            }

            if (any_needs_action(needs_action)) {
                cursor = next_player_to_act(cursor);
            }
        }

        return active_player_count() > 1;
    }

    bool betting_round(std::size_t start_index, std::int64_t current_bet, std::int64_t min_raise, const std::string& street_name, HandLog& log, bool verbose) {
        const auto chooser = [&](std::size_t cursor, std::int64_t to_call, std::int64_t current_bet_now, std::int64_t min_raise_to_now) {
            const auto& player = players_[cursor];
            const double roll = rng_.uniform_real();
            if (to_call > 0 && roll < 0.18) {
                return ForcedAction{ForcedAction::Type::fold, 0};
            }
            const bool can_raise = player.stack > to_call;
            if (can_raise && roll > (to_call > 0 ? 0.78 : 0.88)) {
                std::int64_t target = std::max(min_raise_to_now, current_bet_now + big_blind_);
                target = std::min(target, player.street_commitment + player.stack);
                return ForcedAction{ForcedAction::Type::raise_to, target};
            }
            return ForcedAction{ForcedAction::Type::check_call, 0};
        };

        return betting_round_impl(start_index, current_bet, min_raise, street_name, log, verbose, chooser);
    }

    [[nodiscard]] bool any_needs_action(const std::vector<bool>& needs_action) const noexcept {
        for (std::size_t i = 0; i < needs_action.size(); ++i) {
            if (needs_action[i] && player_can_act(i)) {
                return true;
            }
        }
        return false;
    }

    void reopen_action(std::vector<bool>& needs_action, std::size_t raiser) const {
        for (std::size_t i = 0; i < players_.size(); ++i) {
            needs_action[i] = player_can_act(i);
        }
        needs_action[raiser] = false;
    }

    void apply_street_rake(Street street) {
        if (flop_rake_ <= 0) {
            return;
        }
        switch (street) {
        case Street::flop:
            pot_manager_.take_rake(flop_rake_);
            break;
        case Street::turn:
            pot_manager_.take_rake(1);
            break;
        case Street::river:
            pot_manager_.take_rake(1);
            break;
        case Street::preflop:
            break;
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
        auto stacks = current_stacks();
        pot_manager_.award_even_split(survivors, stacks);
        apply_stacks(stacks);
        if (verbose) {
            log.events.push_back(players_[survivors.front()].name + " wins uncontested pot; rake=" + std::to_string(pot_manager_.rake_this_hand()));
        }
        return log;
    }

    void distribute_showdown_pots(
        std::vector<std::int64_t>& stacks,
        const std::vector<std::int64_t>& contributions,
        const std::vector<bool>& folded,
        const std::vector<HandValue>& hand_values,
        std::vector<std::size_t>* overall_winners = nullptr,
        HandValue* best_value = nullptr,
        bool* has_best = nullptr) {

        auto remaining = contributions;
        while (true) {
            std::int64_t layer = 0;
            for (const auto contribution : remaining) {
                if (contribution > 0 && (layer == 0 || contribution < layer)) {
                    layer = contribution;
                }
            }
            if (layer == 0) {
                break;
            }

            std::vector<std::size_t> contributors;
            std::vector<std::size_t> eligible;
            for (std::size_t i = 0; i < remaining.size(); ++i) {
                if (remaining[i] > 0) {
                    contributors.push_back(i);
                    if (!folded[i]) {
                        eligible.push_back(i);
                    }
                }
            }

            HandValue slice_best {};
            bool slice_has_best = false;
            std::vector<std::size_t> slice_winners;
            for (const auto idx : eligible) {
                if (!slice_has_best || slice_best < hand_values[idx]) {
                    slice_best = hand_values[idx];
                    slice_winners = {idx};
                    slice_has_best = true;
                } else if (hand_values[idx] == slice_best) {
                    slice_winners.push_back(idx);
                }
            }

            const std::int64_t slice_amount = std::min<std::int64_t>(layer * static_cast<std::int64_t>(contributors.size()), pot_manager_.pot());
            if (slice_amount <= 0) {
                break;
            }
            pot_manager_.award_even_split_amount(slice_amount, slice_winners, stacks);

            if (overall_winners != nullptr) {
                for (const auto idx : slice_winners) {
                    if (std::find(overall_winners->begin(), overall_winners->end(), idx) == overall_winners->end()) {
                        overall_winners->push_back(idx);
                    }
                }
            }
            if (best_value != nullptr && has_best != nullptr) {
                if (!*has_best || *best_value < slice_best) {
                    *best_value = slice_best;
                    *has_best = true;
                }
            }

            for (auto& contribution : remaining) {
                if (contribution > 0) {
                    contribution -= layer;
                }
            }
        }
    }

    HandLog showdown(HandLog log, bool verbose) {
        std::vector<HandValue> values(players_.size());
        for (const std::size_t idx : active_players()) {
            std::vector<Card> cards;
            cards.reserve(7);
            cards.push_back(players_[idx].hole_cards[0]);
            cards.push_back(players_[idx].hole_cards[1]);
            cards.insert(cards.end(), board_.begin(), board_.end());
            values[idx] = evaluate_best_of_seven(cards);
            if (verbose) {
                log.events.push_back(players_[idx].name + " shows " + describe_hand(values[idx]));
            }
        }

        auto stacks = current_stacks();
        std::vector<std::size_t> overall_winners;
        HandValue best_value {};
        bool has_best = false;
        std::vector<bool> folded(players_.size(), false);
        for (std::size_t i = 0; i < players_.size(); ++i) {
            folded[i] = players_[i].folded;
        }
        distribute_showdown_pots(stacks, pot_manager_.contributions(), folded, values, &overall_winners, &best_value, &has_best);
        apply_stacks(stacks);

        if (verbose) {
            std::ostringstream oss;
            oss << "Winner" << (overall_winners.size() > 1 ? "s" : "") << ": ";
            for (std::size_t i = 0; i < overall_winners.size(); ++i) {
                if (i) {
                    oss << ", ";
                }
                oss << players_[overall_winners[i]].name;
            }
            if (has_best) {
                oss << " with " << describe_hand(best_value);
            }
            oss << "; rake=" << pot_manager_.rake_this_hand();
            log.events.push_back(oss.str());
        }
        return log;
    }

    std::int64_t small_blind_ {0};
    std::int64_t big_blind_ {0};
    std::int64_t flop_rake_ {0};
    std::int64_t initial_total_chips_ {0};
    std::size_t button_index_ {0};
    std::int64_t current_bet_ {0};
    std::int64_t min_raise_to_ {0};
    Deck deck_;
    PotManager pot_manager_;
    swarm::util::Rng rng_;
    std::vector<PlayerState> players_;
    std::vector<Card> board_;
};

} // namespace swarm::poker
