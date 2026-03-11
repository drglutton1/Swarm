#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include "../src/core/ocean.h"
#include "../src/core/swarm.h"
#include "../src/poker/hand_evaluator.h"
#include "../src/poker/table.h"
#include "../src/util/rng.h"

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

void test_ocean_substrate_behavior() {
    swarm::core::Ocean ocean(8, 0.0f);
    ocean.set(0, 1.0f);
    ocean.set(1, 3.0f);
    ocean.set(7, 5.0f);
    require(ocean.wrap_index(-1) == 7, "ocean wrap index handles negatives");
    require(std::fabs(ocean.window_mean(0, 1) - 3.0f) < 0.0001f, "ocean window mean uses circular neighborhood");
    require(std::fabs(ocean.local_gradient(0) - (3.0f - 5.0f)) < 0.0001f, "ocean local gradient uses adjacent values");
    ocean.diffuse(0.5f);
    ocean.deposit(0, 0.25f);
    require(ocean.get(0) > 1.0f, "ocean deposit updates substrate after diffusion");
}

void test_swarm_birth_and_governance_invariants() {
    swarm::util::Rng rng(20260311);
    swarm::core::Ocean ocean(512);
    ocean.initialize_uniform(-1.0f, 1.0f, rng);
    ocean.diffuse(0.2f);
    ocean.decay(0.99f);

    constexpr std::size_t state_size = 6;
    for (int swarm_index = 0; swarm_index < 200; ++swarm_index) {
        auto swarm = swarm::core::Swarm::random(static_cast<std::uint32_t>(ocean.size()), state_size, rng);
        require(swarm.total_agents() >= 4 && swarm.total_agents() <= 18, "swarm total agents stays within 4-18");
        require(swarm.first_chromosome().size() >= 2 && swarm.first_chromosome().size() <= 9, "first chromosome has valid size");
        require(swarm.second_chromosome().size() >= 2 && swarm.second_chromosome().size() <= 9, "second chromosome has valid size");
        require(swarm.first_chromosome().size() + swarm.second_chromosome().size() == swarm.total_agents(), "chromosome sizes sum to swarm total");
        require(swarm.first_chromosome().blueprint().chromosome_agent_count == swarm.first_chromosome().size(), "first chromosome blueprint matches realized count");
        require(swarm.second_chromosome().blueprint().chromosome_agent_count == swarm.second_chromosome().size(), "second chromosome blueprint matches realized count");
        require(!swarm.first_chromosome().blueprint().sensory_indices.empty(), "first chromosome blueprint has sensory indices");
        require(!swarm.second_chromosome().blueprint().action_indices.empty(), "second chromosome blueprint has action indices");

        const auto expected_mode = (swarm.total_agents() % 2 == 1)
            ? swarm::core::GovernanceMode::alpha_led
            : swarm::core::GovernanceMode::democratic;
        require(swarm.governance_mode() == expected_mode, "governance mode follows total-agent parity");
    }
}

void test_random_swarm_decisions() {
    swarm::util::Rng rng(99);
    swarm::core::Ocean ocean(512);
    ocean.initialize_uniform(-1.0f, 1.0f, rng);
    ocean.diffuse(0.25f);
    ocean.decay(0.995f);

    constexpr std::size_t state_size = 6;
    for (int i = 0; i < 200; ++i) {
        auto swarm = swarm::core::Swarm::random(static_cast<std::uint32_t>(ocean.size()), state_size, rng);
        swarm::core::PokerStateVector state{{
                static_cast<float>(rng.uniform_real(0.0, 1.0)),
                static_cast<float>(rng.uniform_real(0.0, 1.0)),
                static_cast<float>(rng.uniform_real(0.0, 1.0)),
                static_cast<float>(rng.uniform_real(0.0, 1.0)),
                static_cast<float>(rng.uniform_real(0.0, 1.0)),
                static_cast<float>(rng.uniform_real(0.0, 1.0))},
            static_cast<float>(rng.uniform_real(0.0, 20.0)),
            static_cast<float>(rng.uniform_real(2.0, 15.0)),
            static_cast<float>(rng.uniform_real(10.0, 120.0)),
            static_cast<float>(rng.uniform_real(30.0, 200.0))};

        const auto decision = swarm.decide(ocean, state, static_cast<std::size_t>(i + 1));
        const float score_sum = decision.action_scores[0] + decision.action_scores[1] + decision.action_scores[2];
        require(std::fabs(score_sum - 1.0f) < 0.0001f, "action scores sum to 1");
        require(decision.action == swarm::core::ActionType::fold || decision.action == swarm::core::ActionType::check_call || decision.action == swarm::core::ActionType::raise,
            "decision action valid");
        require(decision.confidence >= 0.0f && decision.confidence <= 1.0f, "confidence in range");
        require(decision.raise_amount >= 0.0f, "raise amount non-negative");
        require(decision.raise_amount <= state.stack - state.to_call + 0.0001f, "raise amount within stack cap");
        if (decision.action != swarm::core::ActionType::raise) {
            require(decision.raise_amount == 0.0f, "non-raise action has zero raise amount");
        }
    }
}

} // namespace

int main() {
    test_hand_ordering();
    test_tiebreakers();
    test_chip_conservation();
    test_verbose_flow_mentions_holdem_streets();
    test_heads_up_button_posts_small_blind();
    test_ocean_substrate_behavior();
    test_swarm_birth_and_governance_invariants();
    test_random_swarm_decisions();
    std::cout << "PASS: test_poker\n";
    return 0;
}
