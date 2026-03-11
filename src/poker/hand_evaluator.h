#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "card.h"

namespace swarm::poker {

enum class HandCategory : std::uint8_t {
    high_card = 0,
    one_pair = 1,
    two_pair = 2,
    three_of_a_kind = 3,
    straight = 4,
    flush = 5,
    full_house = 6,
    four_of_a_kind = 7,
    straight_flush = 8
};

struct HandValue {
    HandCategory category {HandCategory::high_card};
    std::array<int, 5> tiebreak {};

    bool operator<(const HandValue& other) const noexcept {
        if (category != other.category) {
            return static_cast<int>(category) < static_cast<int>(other.category);
        }
        return tiebreak < other.tiebreak;
    }

    bool operator==(const HandValue& other) const noexcept {
        return category == other.category && tiebreak == other.tiebreak;
    }
};

inline std::string to_string(HandCategory category) {
    switch (category) {
        case HandCategory::high_card: return "high_card";
        case HandCategory::one_pair: return "one_pair";
        case HandCategory::two_pair: return "two_pair";
        case HandCategory::three_of_a_kind: return "three_of_a_kind";
        case HandCategory::straight: return "straight";
        case HandCategory::flush: return "flush";
        case HandCategory::full_house: return "full_house";
        case HandCategory::four_of_a_kind: return "four_of_a_kind";
        case HandCategory::straight_flush: return "straight_flush";
    }
    return "unknown";
}

inline int highest_straight_rank(const std::array<int, 15>& rank_counts) {
    for (int high = 14; high >= 5; --high) {
        bool is_straight = true;
        for (int offset = 0; offset < 5; ++offset) {
            if (rank_counts[high - offset] == 0) {
                is_straight = false;
                break;
            }
        }
        if (is_straight) {
            return high;
        }
    }
    if (rank_counts[14] && rank_counts[5] && rank_counts[4] && rank_counts[3] && rank_counts[2]) {
        return 5;
    }
    return 0;
}

inline std::array<int, 5> top_ranks_desc(const std::array<int, 15>& rank_counts) {
    std::array<int, 5> values {};
    int index = 0;
    for (int rank = 14; rank >= 2 && index < 5; --rank) {
        for (int count = 0; count < rank_counts[rank] && index < 5; ++count) {
            values[index++] = rank;
        }
    }
    return values;
}

inline HandValue evaluate_five(const std::array<Card, 5>& cards) {
    std::array<int, 15> rank_counts {};
    std::array<int, 4> suit_counts {};
    for (const auto& card : cards) {
        ++rank_counts[card.rank_value()];
        ++suit_counts[card.suit_value()];
    }

    bool is_flush = std::any_of(suit_counts.begin(), suit_counts.end(), [](int c) { return c == 5; });
    int straight_high = highest_straight_rank(rank_counts);

    std::vector<int> quads;
    std::vector<int> trips;
    std::vector<int> pairs;
    std::vector<int> singles;
    for (int rank = 14; rank >= 2; --rank) {
        switch (rank_counts[rank]) {
            case 4: quads.push_back(rank); break;
            case 3: trips.push_back(rank); break;
            case 2: pairs.push_back(rank); break;
            case 1: singles.push_back(rank); break;
            default: break;
        }
    }

    if (is_flush && straight_high) {
        return {HandCategory::straight_flush, {straight_high, 0, 0, 0, 0}};
    }
    if (!quads.empty()) {
        return {HandCategory::four_of_a_kind, {quads.front(), singles.front(), 0, 0, 0}};
    }
    if (!trips.empty() && (!pairs.empty() || trips.size() > 1)) {
        const int trip_rank = trips.front();
        const int pair_rank = !pairs.empty() ? pairs.front() : trips[1];
        return {HandCategory::full_house, {trip_rank, pair_rank, 0, 0, 0}};
    }
    if (is_flush) {
        return {HandCategory::flush, top_ranks_desc(rank_counts)};
    }
    if (straight_high) {
        return {HandCategory::straight, {straight_high, 0, 0, 0, 0}};
    }
    if (!trips.empty()) {
        std::array<int, 5> tie {trips.front(), 0, 0, 0, 0};
        for (std::size_t i = 0; i < singles.size() && i < 2; ++i) {
            tie[i + 1] = singles[i];
        }
        return {HandCategory::three_of_a_kind, tie};
    }
    if (pairs.size() >= 2) {
        return {HandCategory::two_pair, {pairs[0], pairs[1], singles.front(), 0, 0}};
    }
    if (pairs.size() == 1) {
        std::array<int, 5> tie {pairs.front(), 0, 0, 0, 0};
        for (std::size_t i = 0; i < singles.size() && i < 3; ++i) {
            tie[i + 1] = singles[i];
        }
        return {HandCategory::one_pair, tie};
    }
    return {HandCategory::high_card, top_ranks_desc(rank_counts)};
}

inline HandValue evaluate_best_of_seven(const std::vector<Card>& cards) {
    if (cards.size() < 5 || cards.size() > 7) {
        throw std::invalid_argument("evaluate_best_of_seven expects 5-7 cards");
    }

    HandValue best {};
    bool has_best = false;
    const int n = static_cast<int>(cards.size());
    for (int a = 0; a < n - 4; ++a) {
        for (int b = a + 1; b < n - 3; ++b) {
            for (int c = b + 1; c < n - 2; ++c) {
                for (int d = c + 1; d < n - 1; ++d) {
                    for (int e = d + 1; e < n; ++e) {
                        std::array<Card, 5> combo {cards[a], cards[b], cards[c], cards[d], cards[e]};
                        HandValue value = evaluate_five(combo);
                        if (!has_best || best < value) {
                            best = value;
                            has_best = true;
                        }
                    }
                }
            }
        }
    }
    return best;
}

inline std::string describe_hand(const HandValue& value) {
    std::ostringstream oss;
    oss << to_string(value.category) << " [";
    for (std::size_t i = 0; i < value.tiebreak.size(); ++i) {
        if (i) oss << ',';
        oss << value.tiebreak[i];
    }
    oss << ']';
    return oss.str();
}

} // namespace swarm::poker
