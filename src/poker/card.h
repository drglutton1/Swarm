#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>

namespace swarm::poker {

enum class Suit : std::uint8_t { clubs = 0, diamonds = 1, hearts = 2, spades = 3 };
enum class Rank : std::uint8_t {
    two = 2,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    jack,
    queen,
    king,
    ace
};

struct Card {
    Rank rank {};
    Suit suit {};

    constexpr int rank_value() const noexcept { return static_cast<int>(rank); }
    constexpr int suit_value() const noexcept { return static_cast<int>(suit); }
};

inline constexpr std::array<char, 4> SUIT_CHARS {'c', 'd', 'h', 's'};
inline constexpr std::array<char, 15> RANK_CHARS {'?', '?', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'};

inline std::string to_string(const Card& card) {
    std::string out;
    out.push_back(RANK_CHARS[card.rank_value()]);
    out.push_back(SUIT_CHARS[card.suit_value()]);
    return out;
}

inline std::ostream& operator<<(std::ostream& os, const Card& card) {
    return os << to_string(card);
}

} // namespace swarm::poker
