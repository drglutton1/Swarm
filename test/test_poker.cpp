#include <cstdlib>
#include <iostream>
#include <vector>

#include "../src/poker/hand_evaluator.h"
#include "../src/poker/table.h"

using swarm::poker::Card;
using swarm::poker::HandCategory;
using swarm::poker::Rank;
using swarm::poker::Suit;

namespace {

Card c(Rank rank, Suit suit) {
    return Card{rank, suit};
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void test_hand_ordering() {
    const auto straight_flush = swarm::poker::evaluate_best_of_seven({
        c(Rank::ten, Suit::hearts), c(Rank::jack, Suit::hearts), c(Rank::queen, Suit::hearts),
        c(Rank::king, Suit::hearts), c(Rank::ace, Suit::hearts), c(Rank::two, Suit::clubs), c(Rank::three, Suit::diamonds)});
    const auto four_kind = swarm::poker::evaluate_best_of_seven({
        c(Rank::nine, Suit::clubs), c(Rank::nine, Suit::diamonds), c(Rank::nine, Suit::hearts),
        c(Rank::nine, Suit::spades), c(Rank::ace, Suit::clubs), c(Rank::three, Suit::clubs), c(Rank::four, Suit::clubs)});
    const auto full_house = swarm::poker::evaluate_best_of_seven({
        c(Rank::queen, Suit::clubs), c(Rank::queen, Suit::diamonds), c(Rank::queen, Suit::hearts),
        c(Rank::jack, Suit::clubs), c(Rank::jack, Suit::spades), c(Rank::two, Suit::clubs), c(Rank::three, Suit::clubs)});
    const auto straight = swarm::poker::evaluate_best_of_seven({
        c(Rank::ace, Suit::clubs), c(Rank::two, Suit::diamonds), c(Rank::three, Suit::hearts),
        c(Rank::four, Suit::spades), c(Rank::five, Suit::clubs), c(Rank::king, Suit::clubs), c(Rank::queen, Suit::clubs)});

    require(straight_flush.category == HandCategory::straight_flush, "straight flush detected");
    require(four_kind.category == HandCategory::four_of_a_kind, "four of a kind detected");
    require(full_house.category == HandCategory::full_house, "full house detected");
    require(straight.category == HandCategory::straight && straight.tiebreak[0] == 5, "wheel straight detected");
    require(four_kind < straight_flush, "ranking order four kind < straight flush");
    require(full_house < four_kind, "ranking order full house < four kind");
    require(straight < full_house, "ranking order straight < full house");
}

void test_chip_conservation() {
    swarm::poker::Table table(8, 1000, 10, 1, 42);
    const auto summary = table.run(5000, false, std::cout);
    require(summary.initial_chips == 8000, "initial chips tracked");
    require(table.chip_conservation_holds(), "chip conservation holds after simulation");
}

} // namespace

int main() {
    test_hand_ordering();
    test_chip_conservation();
    std::cout << "PASS: test_poker\n";
    return 0;
}
