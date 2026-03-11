#include <cstdlib>
#include <iostream>
#include <sstream>
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

void test_tiebreakers() {
    const auto aces_up = swarm::poker::evaluate_best_of_seven({
        c(Rank::ace, Suit::clubs), c(Rank::ace, Suit::diamonds), c(Rank::king, Suit::hearts),
        c(Rank::king, Suit::spades), c(Rank::queen, Suit::clubs), c(Rank::two, Suit::clubs), c(Rank::three, Suit::clubs)});
    const auto kings_up = swarm::poker::evaluate_best_of_seven({
        c(Rank::king, Suit::clubs), c(Rank::king, Suit::diamonds), c(Rank::queen, Suit::hearts),
        c(Rank::queen, Suit::spades), c(Rank::jack, Suit::clubs), c(Rank::two, Suit::diamonds), c(Rank::three, Suit::diamonds)});
    require(kings_up < aces_up, "two-pair tiebreak ordering works");
}

void test_chip_conservation() {
    swarm::poker::Table table(8, 1000, 5, 5, 1, 42);
    const auto summary = table.run(2000, false, std::cout);
    require(summary.initial_chips == 8000, "initial chips tracked");
    require(summary.hands_played > 0, "simulation played at least one hand");
    require(table.chip_conservation_holds(), "chip conservation holds after simulation");
}

void test_verbose_flow_mentions_holdem_streets() {
    swarm::poker::Table table(6, 200, 5, 5, 0, 1337);
    std::ostringstream oss;
    const auto summary = table.run(3, true, oss);
    const auto text = oss.str();
    require(summary.hands_played > 0, "verbose run played hands");
    require(text.find("posts small blind 5") != std::string::npos, "small blind logged at 5");
    require(text.find("posts big blind 5") != std::string::npos, "big blind logged at 5");
    require(text.find("Preflop:") != std::string::npos, "preflop action logged");
    require(text.find("Flop:") != std::string::npos, "flop logged");
}

void test_heads_up_button_posts_small_blind() {
    swarm::poker::Table table(2, 100, 5, 5, 0, 7);
    std::ostringstream oss;
    const auto summary = table.run(1, true, oss);
    const auto text = oss.str();
    require(summary.hands_played == 1, "heads-up hand played");
    const auto dealer_pos = text.find("Dealer: Bot1");
    const auto sb_pos = text.find("Bot1 posts small blind 5");
    const auto bb_pos = text.find("Bot2 posts big blind 5");
    require(dealer_pos != std::string::npos, "dealer logged in heads-up hand");
    require(sb_pos != std::string::npos, "dealer posts small blind heads-up");
    require(bb_pos != std::string::npos, "other player posts big blind heads-up");
    require(dealer_pos < sb_pos && sb_pos < bb_pos, "heads-up blind order is correct");
}

} // namespace

int main() {
    test_hand_ordering();
    test_tiebreakers();
    test_chip_conservation();
    test_verbose_flow_mentions_holdem_streets();
    test_heads_up_button_posts_small_blind();
    std::cout << "PASS: test_poker\n";
    return 0;
}
