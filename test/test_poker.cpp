#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include "../src/core/ocean.h"
#include "../src/core/swarm.h"
#include "../src/economy/chip_manager.h"
#include "../src/economy/inheritance.h"
#include "../src/economy/transfer.h"
#include "../src/evolution/lifecycle.h"
#include "../src/evolution/reproduction.h"
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

swarm::core::Chromosome make_chromosome(std::size_t agent_count, std::size_t state_size, std::uint32_t ocean_size, swarm::util::Rng& rng) {
    const swarm::core::DecoderConfig config{state_size, 4, 3};
    auto genome = swarm::core::Genome::random(state_size, ocean_size, rng, static_cast<std::uint32_t>(agent_count));
    return swarm::core::Chromosome::spawn_from_blueprint(genome, agent_count, config, ocean_size);
}

swarm::core::Swarm make_swarm(std::size_t first_agents, std::size_t second_agents, swarm::util::Rng& rng) {
    constexpr std::size_t state_size = 6;
    constexpr std::uint32_t ocean_size = 512;
    return swarm::core::Swarm(
        make_chromosome(first_agents, state_size, ocean_size, rng),
        make_chromosome(second_agents, state_size, ocean_size, rng),
        swarm::core::Swarm::starting_bankroll);
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
        require(swarm.bankroll() == swarm::core::Swarm::starting_bankroll, "random swarm starts with standard bankroll");
        require(swarm.alive(), "random swarm starts alive");

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

void test_economy_accounting_and_inheritance() {
    swarm::util::Rng rng(20260312);
    constexpr std::size_t state_size = 6;

    auto founder = swarm::core::Swarm::random(512, state_size, rng);
    auto child_a = swarm::core::Swarm::random(512, state_size, rng);
    auto child_b = swarm::core::Swarm::random(512, state_size, rng);
    auto child_c = swarm::core::Swarm::random(512, state_size, rng);

    founder.remove_bankroll(founder.bankroll());
    child_a.remove_bankroll(child_a.bankroll());
    child_b.remove_bankroll(child_b.bankroll());
    child_c.remove_bankroll(child_c.bankroll());

    swarm::economy::ChipManager manager;
    manager.inject(founder, swarm::core::Swarm::starting_bankroll);
    manager.inject(child_a, swarm::core::Swarm::starting_bankroll);
    manager.inject(child_b, swarm::core::Swarm::starting_bankroll);
    manager.inject(child_c, swarm::core::Swarm::starting_bankroll);
    require(manager.chips_injected() == 20000, "chip manager tracks total injected chips");
    require(manager.chips_in_play() == 20000, "all injected chips start in play");

    const auto transfer_one = swarm::economy::TransferService::execute(founder, child_a, 1200, manager);
    const auto transfer_two = swarm::economy::TransferService::execute(child_b, child_c, 700, manager);
    require(transfer_one.amount == 1200, "transfer records amount");
    require(transfer_two.amount == 700, "second transfer records amount");
    require(manager.chips_in_play() == 20000, "transfers are accounting-neutral");

    child_b.mark_dead();
    const auto inheritance = swarm::economy::InheritanceService::distribute_on_death(
        founder,
        std::vector<swarm::core::Swarm*>{&child_a, &child_b, &child_c},
        manager);

    require(!founder.alive(), "founder marked dead after inheritance event");
    require(child_b.bankroll() == 4300, "unrelated dead offspring keeps prior bankroll untouched");
    require(inheritance.starting_bankroll == 3800, "inheritance sees founder post-transfer bankroll");
    require(inheritance.burned == 1900, "inheritance burns half when living offspring exist");
    require(inheritance.distributed == 1900, "remaining half distributed to living offspring");
    require(inheritance.payouts.size() == 2, "only living offspring receive payouts");
    require(inheritance.payouts[0] == 950 && inheritance.payouts[1] == 950, "living offspring split inheritance equally");
    require(child_a.bankroll() == 7150, "first living child receives inheritance payout");
    require(child_c.bankroll() == 6650, "second living child receives inheritance payout");
    require(manager.chips_burned() == 1900, "burned chips tracked after inheritance");
    require(manager.chips_in_play() == 18100, "chips in play reduced only by burned amount");
    require(manager.invariants_hold(), "economy invariant holds after transfer and inheritance scenario");
    require(manager.chips_in_play() + manager.chips_burned() == manager.chips_injected(), "core economy identity holds");

    auto lone_parent = swarm::core::Swarm::random(512, state_size, rng);
    lone_parent.remove_bankroll(lone_parent.bankroll());
    manager.inject(lone_parent, swarm::core::Swarm::starting_bankroll);
    const auto no_offspring = swarm::economy::InheritanceService::distribute_on_death(lone_parent, {}, manager);
    require(no_offspring.burned == 5000, "no living offspring means full burn");
    require(no_offspring.distributed == 0, "no offspring means no payouts");
    require(manager.chips_burned() == 6900, "burn total accumulates across multiple deaths");
    require(manager.chips_in_play() == 18100, "full-burn death removes all newly injected chips from play");
    require(manager.chips_in_play() + manager.chips_burned() == manager.chips_injected(), "economy invariant still holds after full burn");
}

void test_lifecycle_phase_transitions() {
    swarm::util::Rng rng(11);
    auto swarm = make_swarm(2, 2, rng);

    auto state = swarm::evolution::evaluate(swarm);
    require(state.phase == swarm::evolution::LifePhase::youth, "new swarm starts in youth");
    require(!state.reproduction_window_open, "youth cannot reproduce");

    swarm.set_hands_played(10000);
    state = swarm::evolution::evaluate(swarm);
    require(state.phase == swarm::evolution::LifePhase::maturity, "swarm enters maturity at 10k hands");
    require(state.reproduction_window_open, "mature swarm can reproduce when eligible");

    swarm.set_hands_played(90000);
    state = swarm::evolution::evaluate(swarm);
    require(state.phase == swarm::evolution::LifePhase::old_age, "swarm enters old age at 90k hands");
    require(!state.reproduction_window_open, "old age reproduction is disabled");

    swarm.set_hands_played(100000);
    state = swarm::evolution::evaluate(swarm);
    require(state.phase == swarm::evolution::LifePhase::dead, "swarm dies at 100k hands");
    require(!state.alive, "dead state is not alive");
}

void test_even_odd_pairing_is_enforced() {
    swarm::util::Rng rng(22);
    auto even_swarm = make_swarm(2, 2, rng);
    auto odd_swarm = make_swarm(2, 3, rng);
    auto even_swarm_two = make_swarm(3, 3, rng);

    even_swarm.set_hands_played(20000);
    odd_swarm.set_hands_played(20000);
    even_swarm_two.set_hands_played(20000);

    require(swarm::evolution::pairing_parity_allowed(even_swarm, odd_swarm), "even plus odd pairing allowed");
    require(!swarm::evolution::pairing_parity_allowed(even_swarm, even_swarm_two), "even plus even pairing rejected");
}

void test_invalid_parents_cannot_reproduce() {
    swarm::util::Rng rng(33);
    auto youth = make_swarm(2, 2, rng);
    auto mature = make_swarm(2, 3, rng);
    mature.set_hands_played(20000);

    auto result = swarm::evolution::reproduce(youth, mature, 6, 512, rng);
    require(!result.success, "youth parent cannot reproduce");

    youth.set_hands_played(20000);
    youth.note_reproduction();
    youth.record_hands(1000);
    result = swarm::evolution::reproduce(youth, mature, 6, 512, rng);
    require(!result.success, "cooldown blocks reproduction");

    auto bankrupt = make_swarm(2, 2, rng);
    bankrupt.set_hands_played(20000);
    bankrupt.remove_bankroll(bankrupt.bankroll());
    result = swarm::evolution::reproduce(bankrupt, mature, 6, 512, rng);
    require(!result.success, "dead bankroll parent cannot reproduce");
}

void test_offspring_is_structurally_valid_and_starts_correctly() {
    swarm::util::Rng rng(44);
    auto even_parent = make_swarm(2, 2, rng);
    auto odd_parent = make_swarm(2, 3, rng);
    even_parent.set_hands_played(30000);
    odd_parent.set_hands_played(32000);

    swarm::evolution::ReproductionPolicy policy;
    policy.crossover.crossover_probability = 1.0;
    policy.mutation.point_mutation_rate = 1.0;
    policy.mutation.meta_mutation_rate = 1.0;
    policy.mutation.structural_mutation_rate = 1.0;

    const auto result = swarm::evolution::reproduce(even_parent, odd_parent, 6, 512, rng, policy);
    require(result.success, "valid parents produce offspring");
    require(result.offspring.has_value(), "offspring returned");

    const auto& child = *result.offspring;
    require(child.bankroll() == swarm::core::Swarm::starting_bankroll, "offspring starts with 5000 bankroll");
    require(child.alive(), "offspring starts alive");
    require(child.hands_played() == 0, "offspring starts at zero hands played");
    require(child.first_chromosome().size() >= 2 && child.first_chromosome().size() <= 9, "offspring first chromosome size valid");
    require(child.second_chromosome().size() >= 2 && child.second_chromosome().size() <= 9, "offspring second chromosome size valid");
    require(child.total_agents() >= 4 && child.total_agents() <= 18, "offspring total agents valid");
    require(child.first_chromosome().blueprint().chromosome_agent_count == child.first_chromosome().size(), "offspring first blueprint count matches size");
    require(child.second_chromosome().blueprint().chromosome_agent_count == child.second_chromosome().size(), "offspring second blueprint count matches size");
    require(!child.first_chromosome().blueprint().sensory_indices.empty(), "offspring first blueprint keeps sensory structure");
    require(!child.second_chromosome().blueprint().action_indices.empty(), "offspring second blueprint keeps action structure");
    require(even_parent.offspring_count() == 1 && odd_parent.offspring_count() == 1, "parents record offspring count");
    require(even_parent.last_reproduction_hand() == even_parent.hands_played(), "first parent reproduction timestamp updated");
    require(odd_parent.last_reproduction_hand() == odd_parent.hands_played(), "second parent reproduction timestamp updated");
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
    test_economy_accounting_and_inheritance();
    test_lifecycle_phase_transitions();
    test_even_odd_pairing_is_enforced();
    test_invalid_parents_cannot_reproduce();
    test_offspring_is_structurally_valid_and_starts_correctly();
    std::cout << "PASS: test_poker\n";
    return 0;
}
