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
#include "../src/social/face.h"
#include "../src/social/info_exchange.h"
#include "../src/social/mate_selection.h"
#include "../src/poker/hand_evaluator.h"
#include "../src/poker/table.h"
#include "../src/scheduler/time_manager.h"
#include "../src/scheduler/activity_scheduler.h"
#include "../src/scheduler/table_manager.h"
#include "../src/sim/simulation.h"
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

swarm::core::Genome make_scripted_genome(
    std::uint32_t dominant_action_slot,
    float alpha_bias,
    float confidence_gene,
    float risk_gene,
    float base_strength = 6.0f,
    float confidence_signal = 4.0f,
    float raise_signal = 3.0f) {
    swarm::core::Genome genome;
    genome.hidden_units = 1;
    genome.action_count = 3;
    genome.ocean_window_radius = 1;
    genome.chromosome_agent_count = 1;
    genome.alpha_bias = alpha_bias;
    genome.confidence_gene = confidence_gene;
    genome.risk_gene = risk_gene;
    genome.honesty_gene = 0.5f;
    genome.skepticism_gene = 0.5f;
    genome.sensory_indices = {0};
    genome.hidden_indices = {1, 2};

    const std::uint32_t low_base = 3;
    const std::uint32_t high_base = 11;
    const std::uint32_t low_weight = 4;
    const std::uint32_t high_weight = 12;
    genome.action_indices = {
        dominant_action_slot == 0 ? high_base : low_base,
        dominant_action_slot == 0 ? high_weight : low_weight,
        dominant_action_slot == 1 ? high_base : low_base,
        dominant_action_slot == 1 ? high_weight : low_weight,
        dominant_action_slot == 2 ? high_base : low_base,
        dominant_action_slot == 2 ? high_weight : low_weight,
    };

    const std::uint32_t weak_raise_signal = 13;
    const std::uint32_t strong_raise_signal = 14;
    const std::uint32_t weak_confidence_signal = 13;
    const std::uint32_t strong_confidence_signal = 15;
    genome.modulation_indices = {
        dominant_action_slot == 2 ? strong_raise_signal : weak_raise_signal,
        confidence_gene >= 0.6f ? strong_confidence_signal : weak_confidence_signal,
        13,
        13,
    };
    return genome;
}

swarm::core::Chromosome make_scripted_chromosome(
    const std::vector<std::uint32_t>& dominant_actions,
    float alpha_bias,
    float confidence_gene,
    float risk_gene) {
    const swarm::core::DecoderConfig config{1, 1, 3};
    swarm::core::Genome blueprint = make_scripted_genome(dominant_actions.empty() ? 1U : dominant_actions.front(), alpha_bias, confidence_gene, risk_gene);
    blueprint.chromosome_agent_count = static_cast<std::uint32_t>(dominant_actions.size());

    std::vector<swarm::core::Agent> agents;
    agents.reserve(dominant_actions.size());
    for (std::uint32_t action_slot : dominant_actions) {
        auto genome = make_scripted_genome(action_slot, alpha_bias, confidence_gene, risk_gene);
        agents.emplace_back(genome, config);
    }
    return swarm::core::Chromosome(std::move(agents), blueprint);
}

swarm::core::Ocean make_scripted_ocean() {
    swarm::core::Ocean ocean(16, 0.0f);
    ocean.set(1, 0.2f);
    ocean.set(2, 0.0f);
    ocean.set(3, -4.0f);
    ocean.set(4, -0.2f);
    ocean.set(11, 4.5f);
    ocean.set(12, 0.8f);
    ocean.set(13, -3.0f);
    ocean.set(14, 4.0f);
    ocean.set(15, 3.5f);
    return ocean;
}

swarm::core::PokerStateVector make_scripted_state() {
    swarm::core::PokerStateVector state;
    state.features = {0.0f};
    state.to_call = 1.0f;
    state.min_raise = 2.0f;
    state.pot = 10.0f;
    state.stack = 20.0f;
    return state;
}

swarm::core::Chromosome make_chromosome_with_genes(
    std::size_t agent_count,
    std::size_t state_size,
    std::uint32_t ocean_size,
    swarm::util::Rng& rng,
    float honesty_gene,
    float skepticism_gene,
    float confidence_gene = 0.5f,
    float risk_gene = 0.5f) {
    const swarm::core::DecoderConfig config{state_size, 4, 3};
    auto genome = swarm::core::Genome::random(state_size, ocean_size, rng, static_cast<std::uint32_t>(agent_count));
    genome.honesty_gene = honesty_gene;
    genome.skepticism_gene = skepticism_gene;
    genome.confidence_gene = confidence_gene;
    genome.risk_gene = risk_gene;
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

swarm::core::Swarm make_social_swarm(
    std::size_t first_agents,
    std::size_t second_agents,
    swarm::util::Rng& rng,
    float honesty_gene,
    float skepticism_gene,
    float confidence_gene = 0.5f,
    float risk_gene = 0.5f) {
    constexpr std::size_t state_size = 6;
    constexpr std::uint32_t ocean_size = 512;
    return swarm::core::Swarm(
        make_chromosome_with_genes(first_agents, state_size, ocean_size, rng, honesty_gene, skepticism_gene, confidence_gene, risk_gene),
        make_chromosome_with_genes(second_agents, state_size, ocean_size, rng, honesty_gene, skepticism_gene, confidence_gene, risk_gene),
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

void test_side_pot_eligibility_uneven_all_in() {
    swarm::poker::Table table(3, 100, 5, 5, 0, 1);
    std::vector<std::int64_t> stacks{0, 0, 0};
    std::vector<std::int64_t> contributions{20, 50, 100};
    std::vector<bool> folded{false, false, false};

    const auto best_short = swarm::poker::evaluate_best_of_seven({
        c(Rank::ace, Suit::hearts), c(Rank::ace, Suit::clubs), c(Rank::king, Suit::hearts),
        c(Rank::queen, Suit::hearts), c(Rank::jack, Suit::hearts), c(Rank::nine, Suit::clubs), c(Rank::two, Suit::diamonds)});
    const auto middle = swarm::poker::evaluate_best_of_seven({
        c(Rank::king, Suit::spades), c(Rank::king, Suit::clubs), c(Rank::queen, Suit::spades),
        c(Rank::jack, Suit::spades), c(Rank::ten, Suit::spades), c(Rank::eight, Suit::clubs), c(Rank::three, Suit::diamonds)});
    const auto deep = swarm::poker::evaluate_best_of_seven({
        c(Rank::queen, Suit::clubs), c(Rank::queen, Suit::diamonds), c(Rank::jack, Suit::clubs),
        c(Rank::ten, Suit::clubs), c(Rank::nine, Suit::clubs), c(Rank::seven, Suit::diamonds), c(Rank::four, Suit::spades)});

    const auto result = table.resolve_showdown_for_test(stacks, contributions, folded, {best_short, middle, deep});
    require(result[0] == 60, "short all-in winner gets only main pot");
    require(result[1] == 60, "middle stack wins first side pot");
    require(result[2] == 50, "deep stack keeps final uncontested side pot only");
}

void test_betting_round_allows_reraises_and_reopens_action() {
    swarm::poker::Table table(3, 200, 5, 10, 0, 2);
    const auto result = table.run_forced_betting_round_for_test(
        {100, 100, 100},
        {0, 5, 10},
        {false, false, false},
        0,
        10,
        10,
        "Preflop",
        {
            {swarm::poker::ForcedAction::Type::raise_to, 30},
            {swarm::poker::ForcedAction::Type::raise_to, 50},
            {swarm::poker::ForcedAction::Type::check_call, 0},
            {swarm::poker::ForcedAction::Type::check_call, 0},
        });

    require(result.hand_continues, "multi-raise round still continues to showdown");
    require(result.current_bet == 50, "current bet reflects reraise");
    require(result.players[0].street_commitment == 50, "opener can act again after reraise and calls");
    require(result.players[1].street_commitment == 50, "small blind reraises to 50");
    require(result.players[2].street_commitment == 50, "big blind calls final bet");
}

void test_short_stack_calls_all_in_instead_of_forced_fold() {
    swarm::poker::Table table(3, 200, 5, 10, 0, 3);
    const auto result = table.run_forced_betting_round_for_test(
        {15, 100, 100},
        {0, 0, 0},
        {false, false, false},
        0,
        30,
        10,
        "Turn",
        {
            {swarm::poker::ForcedAction::Type::check_call, 0},
            {swarm::poker::ForcedAction::Type::check_call, 0},
            {swarm::poker::ForcedAction::Type::check_call, 0},
        });

    require(result.players[0].street_commitment == 15, "short stack puts last chips in rather than folding");
    require(!result.players[0].folded, "short stack remains eligible after all-in call");
}

void test_rake_by_street_and_chop_logic() {
    swarm::poker::Table table(2, 100, 5, 5, 5, 4);
    const auto chopped = table.resolve_showdown_for_test(
        {0, 0},
        {50, 50},
        {false, false},
        {
            swarm::poker::evaluate_best_of_seven({c(Rank::ace, Suit::clubs), c(Rank::king, Suit::clubs), c(Rank::queen, Suit::clubs), c(Rank::jack, Suit::clubs), c(Rank::ten, Suit::clubs), c(Rank::two, Suit::hearts), c(Rank::three, Suit::hearts)}),
            swarm::poker::evaluate_best_of_seven({c(Rank::ace, Suit::spades), c(Rank::king, Suit::spades), c(Rank::queen, Suit::spades), c(Rank::jack, Suit::spades), c(Rank::ten, Suit::spades), c(Rank::two, Suit::diamonds), c(Rank::three, Suit::diamonds)})
        },
        7);
    require(chopped[0] == 47 && chopped[1] == 46, "river chop applies cumulative rake then even-split remainder rule");

    const auto unraked_preflop = table.resolve_showdown_for_test(
        {0, 0},
        {5, 5},
        {false, true},
        {
            swarm::poker::evaluate_best_of_seven({c(Rank::ace, Suit::clubs), c(Rank::king, Suit::clubs), c(Rank::queen, Suit::clubs), c(Rank::jack, Suit::clubs), c(Rank::ten, Suit::clubs), c(Rank::two, Suit::hearts), c(Rank::three, Suit::hearts)}),
            swarm::poker::evaluate_best_of_seven({c(Rank::two, Suit::clubs), c(Rank::three, Suit::clubs), c(Rank::four, Suit::clubs), c(Rank::five, Suit::clubs), c(Rank::six, Suit::clubs), c(Rank::seven, Suit::hearts), c(Rank::eight, Suit::hearts)})
        },
        0);
    require(unraked_preflop[0] == 10, "preflop-only pots can settle without street rake");
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

void test_social_info_exchange_truthful_vs_deceptive() {
    swarm::util::Rng truthful_rng(123);
    auto requester = make_social_swarm(2, 2, truthful_rng, 0.5f, 0.2f);
    auto truthful = make_social_swarm(2, 3, truthful_rng, 1.0f, 0.2f);
    truthful.set_hands_played(20000);

    const auto honest_answer = swarm::social::request_private_info(requester, truthful, swarm::social::PrivateInfoField::bankroll, truthful_rng);
    require(!honest_answer.refused, "fully honest swarm answers bankroll request");
    require(honest_answer.truthful, "fully honest swarm answers truthfully");
    require(std::fabs(honest_answer.reported_value - honest_answer.actual_value) < 0.0001, "truthful answer matches reality");

    bool observed_dishonest_behavior = false;
    bool observed_lie = false;
    for (std::uint64_t seed = 456; seed < 556; ++seed) {
        swarm::util::Rng deceptive_rng(seed);
        auto deceptive = make_social_swarm(2, 3, deceptive_rng, 0.0f, 0.2f, 0.6f, 0.9f);
        deceptive.set_hands_played(20000);
        deceptive.add_bankroll(3000);
        const auto answer = swarm::social::request_private_info(requester, deceptive, swarm::social::PrivateInfoField::bankroll, deceptive_rng);
        if (!answer.truthful) {
            observed_dishonest_behavior = true;
        }
        if (!answer.truthful && !answer.refused && std::fabs(answer.reported_value - answer.actual_value) > 0.001) {
            observed_lie = true;
            break;
        }
    }

    require(observed_dishonest_behavior, "dishonest swarm no longer always answers truthfully");
    require(observed_lie, "dishonest swarm can still fabricate bankroll reports");
}

void test_social_skepticism_changes_interpretation() {
    swarm::util::Rng rng(789);
    auto gullible = make_social_swarm(2, 2, rng, 0.5f, 0.0f);
    auto skeptical = make_social_swarm(2, 2, rng, 0.5f, 1.0f);

    swarm::social::InfoResponse lie;
    lie.actual_value = 5000.0;
    lie.reported_value = 9000.0;

    const auto gullible_view = swarm::social::interpret_response(gullible, lie);
    const auto skeptical_view = swarm::social::interpret_response(skeptical, lie);

    require(std::fabs(gullible_view.believed_value - 9000.0) < 0.0001, "zero skepticism fully trusts reported value");
    require(std::fabs(skeptical_view.believed_value - 5000.0) < 0.0001, "max skepticism falls back to baseline actual value");
    require(gullible_view.trust_weight > skeptical_view.trust_weight, "skepticism lowers trust weight");
}

void test_alpha_led_governance_uses_advisor_signal() {
    const auto ocean = make_scripted_ocean();
    const auto state = make_scripted_state();

    swarm::core::Swarm alpha_following_raise(
        make_scripted_chromosome({1, 1, 1}, 0.15f, 0.85f, 0.25f),
        make_scripted_chromosome({2, 2}, 0.40f, 0.80f, 0.95f),
        swarm::core::Swarm::starting_bankroll);

    swarm::core::Swarm alpha_following_fold(
        make_scripted_chromosome({1, 1, 1}, 0.15f, 0.85f, 0.25f),
        make_scripted_chromosome({0, 0}, 0.40f, 0.80f, 0.20f),
        swarm::core::Swarm::starting_bankroll);

    const auto raise_decision = alpha_following_raise.decide(ocean, state, 1);
    const auto fold_decision = alpha_following_fold.decide(ocean, state, 1);

    require(alpha_following_raise.governance_mode() == swarm::core::GovernanceMode::alpha_led, "odd-sized swarm stays alpha-led");
    require(raise_decision.action == swarm::core::ActionType::check_call, "alpha still owns the final call in alpha-led mode");
    require(fold_decision.action == swarm::core::ActionType::check_call, "alpha-led can remain distinct from pure majority voting");
    require(raise_decision.action_scores[2] > fold_decision.action_scores[2] + 0.15f, "raise advisors materially increase alpha-led raise pressure");
    require(fold_decision.action_scores[0] > raise_decision.action_scores[0] + 0.15f, "fold advisors materially increase alpha-led fold pressure");
}

void test_alpha_led_remains_distinct_from_democratic() {
    const auto ocean = make_scripted_ocean();
    const auto state = make_scripted_state();

    swarm::core::Swarm alpha_led(
        make_scripted_chromosome({1, 2, 2}, 0.90f, 0.85f, 0.25f),
        make_scripted_chromosome({2, 2}, 0.40f, 0.80f, 0.95f),
        swarm::core::Swarm::starting_bankroll);

    swarm::core::Swarm democratic(
        make_scripted_chromosome({1, 2, 2}, 0.90f, 0.85f, 0.25f),
        make_scripted_chromosome({2, 2, 2}, 0.40f, 0.80f, 0.95f),
        swarm::core::Swarm::starting_bankroll);

    const auto alpha_decision = alpha_led.decide(ocean, state, 1);
    const auto democratic_decision = democratic.decide(ocean, state, 1);

    require(alpha_led.governance_mode() == swarm::core::GovernanceMode::alpha_led, "odd-sized scripted swarm is alpha-led");
    require(democratic.governance_mode() == swarm::core::GovernanceMode::democratic, "even-sized scripted swarm is democratic");
    require(alpha_decision.action == swarm::core::ActionType::check_call, "alpha-led still respects alpha veto and does not collapse into plain majority rule");
    require(democratic_decision.action == swarm::core::ActionType::raise, "democratic mode follows chromosome aggregate majority");
}

void test_rng_seed_reproducibility_and_ranges() {
    swarm::util::Rng first(20260312);
    swarm::util::Rng second(20260312);
    swarm::util::Rng different(20260313);

    for (int i = 0; i < 32; ++i) {
        require(first.next_u64() == second.next_u64(), "same RNG seed reproduces sequence");
    }

    const auto first_value = first.next_u64();
    const auto different_value = different.next_u64();
    require(first_value != different_value, "different RNG seeds diverge quickly");

    for (int i = 0; i < 256; ++i) {
        const int integer = first.next_int(-3, 7);
        const float real = first.next_float();
        require(integer >= -3 && integer <= 7, "next_int stays within inclusive bounds");
        require(real >= 0.0f && real < 1.0f, "next_float stays within unit interval");
    }

    std::vector<int> shuffled{1, 2, 3, 4, 5, 6, 7, 8};
    swarm::util::Rng shuffle_a(77);
    swarm::util::Rng shuffle_b(77);
    shuffle_a.shuffle(shuffled.begin(), shuffled.end());
    std::vector<int> shuffled_again{1, 2, 3, 4, 5, 6, 7, 8};
    shuffle_b.shuffle(shuffled_again.begin(), shuffled_again.end());
    require(shuffled == shuffled_again, "shuffle is deterministic under the same seed");
}

void test_default_policy_alignment() {
    swarm::evolution::LifecyclePolicy lifecycle;
    swarm::evolution::CrossoverPolicy crossover;
    require(lifecycle.reproduction_cooldown_hands == 10000, "default reproduction cooldown aligned to spec");
    require(std::fabs(crossover.crossover_probability - 0.01) < 1e-9, "default crossover probability aligned to spec");
}

void test_swarm_birth_distribution_sum_of_two_dice() {
    swarm::util::Rng rng(20260312);
    bool saw_four = false;
    bool saw_eighteen = false;
    bool saw_odd = false;
    for (int i = 0; i < 500; ++i) {
        auto swarm = swarm::core::Swarm::random(512, 6, rng);
        const auto first = swarm.first_chromosome().size();
        const auto second = swarm.second_chromosome().size();
        require(first >= 2 && first <= 9, "first chromosome uses 2..9 distribution");
        require(second >= 2 && second <= 9, "second chromosome uses 2..9 distribution");
        require(swarm.total_agents() == first + second, "total agents are the sum of both chromosome draws");
        saw_four = saw_four || swarm.total_agents() == 4;
        saw_eighteen = saw_eighteen || swarm.total_agents() == 18;
        saw_odd = saw_odd || (swarm.total_agents() % 2 == 1);
    }
    require(saw_four, "distribution can realize minimum total 4");
    require(saw_eighteen, "distribution can realize maximum total 18");
    require(saw_odd, "sum-of-two-dice distribution includes odd totals");
}

void test_mate_selection_respects_eligibility_and_parity() {
    swarm::util::Rng rng(2468);
    auto seeker = make_social_swarm(2, 2, rng, 0.5f, 0.2f);
    auto best = make_social_swarm(2, 3, rng, 0.9f, 0.1f);
    auto same_parity = make_social_swarm(3, 3, rng, 0.9f, 0.1f);
    auto immature = make_social_swarm(2, 3, rng, 0.9f, 0.1f);
    auto dead = make_social_swarm(2, 3, rng, 0.9f, 0.1f);

    seeker.set_hands_played(25000);
    best.set_hands_played(25000);
    same_parity.set_hands_played(25000);
    immature.set_hands_played(5000);
    dead.set_hands_played(25000);
    dead.remove_bankroll(dead.bankroll());

    best.add_bankroll(4000);

    const auto best_bankroll = swarm::social::request_private_info(seeker, best, swarm::social::PrivateInfoField::bankroll, rng);
    const auto best_ready = swarm::social::request_private_info(seeker, best, swarm::social::PrivateInfoField::reproductive_readiness, rng);

    std::vector<const swarm::core::Swarm*> candidates{&same_parity, &immature, &dead, &best};
    std::vector<swarm::social::SocialKnowledge> knowledge{
        {swarm::social::make_public_face(same_parity), std::nullopt, std::nullopt, std::nullopt},
        {swarm::social::make_public_face(immature), std::nullopt, std::nullopt, std::nullopt},
        {swarm::social::make_public_face(dead), std::nullopt, std::nullopt, std::nullopt},
        {swarm::social::make_public_face(best), swarm::social::interpret_response(seeker, best_bankroll), swarm::social::interpret_response(seeker, best_ready), std::nullopt}};

    const auto selection = swarm::social::select_partner(seeker, candidates, knowledge);
    require(selection.partner != nullptr, "at least one eligible partner selected");
    require(selection.partner->id() == best.id(), "mate selection prefers eligible parity-compatible mature partner");
}

void test_time_manager_tracks_ticks_and_hand_blocks() {
    swarm::scheduler::TimeManager time({25, 5});
    auto now = time.advance_tick(3);
    require(now.tick == 3, "time manager advances ticks");
    require(now.hand_block == 0, "hand block stays put within block window");
    require(now.tick_in_block == 3, "tick within block is tracked");

    now = time.advance_tick(2);
    require(now.tick == 5, "time manager accumulates ticks");
    require(now.hand_block == 1, "tick rollover advances hand block");
    require(now.hands_elapsed == 25, "hands elapsed follows completed hand blocks");
    require(now.tick_in_block == 0, "tick in block wraps after rollover");

    now = time.advance_hand_blocks(2);
    require(now.hand_block == 3, "explicit hand block advance works");
    require(now.hands_elapsed == 75, "explicit hand block advance updates hands elapsed");
}

void test_scheduler_enforces_minimum_sleep_ratio() {
    swarm::util::Rng rng(9001);
    auto swarm = make_swarm(2, 3, rng);
    swarm.set_hands_played(20000);

    swarm::scheduler::ActivityScheduler scheduler;
    swarm::scheduler::ActivityState state;
    swarm::scheduler::TimeManager time;
    for (int tick = 0; tick < 10; ++tick) {
        const auto lifecycle = swarm::evolution::evaluate(swarm);
        state = scheduler.next_state(swarm, lifecycle, time.now(), tick == 0 ? nullptr : &state);
        time.advance_tick();
    }

    require(state.total_ticks == 10, "activity scheduler tracks total ticks");
    require(state.sleep_ticks >= 3, "activity scheduler enforces at least 30 percent sleep per cycle");
}

void test_table_manager_prevents_double_booking_and_fills_full_table() {
    swarm::util::Rng rng(777);
    std::vector<swarm::core::Swarm> swarms;
    swarms.reserve(8);
    std::vector<swarm::scheduler::SchedulingPassEntry> entries;
    entries.reserve(8);
    for (int i = 0; i < 8; ++i) {
        auto& swarm = swarms.emplace_back(make_swarm(2, 2, rng));
        swarm.set_hands_played(20000);
        entries.push_back({&swarm, swarm::scheduler::ActivityMode::play, swarm::evolution::evaluate(swarm)});
    }

    swarm::scheduler::TableManager manager;
    const auto plan = manager.build_plan(entries);
    require(plan.tables.size() == 1, "eight eligible swarms form one table");
    require(plan.tables.front().swarm_ids.size() == 8, "full table targets eight swarms");

    std::vector<std::uint64_t> assigned = plan.tables.front().swarm_ids;
    std::sort(assigned.begin(), assigned.end());
    require(std::adjacent_find(assigned.begin(), assigned.end()) == assigned.end(), "no swarm is double-booked in a pass");
}

void test_table_manager_handles_leftovers_gracefully() {
    swarm::util::Rng rng(888);
    std::vector<swarm::core::Swarm> swarms;
    swarms.reserve(17);
    std::vector<swarm::scheduler::SchedulingPassEntry> entries;
    entries.reserve(17);
    for (int i = 0; i < 17; ++i) {
        auto& swarm = swarms.emplace_back(make_swarm(2, 3, rng));
        swarm.set_hands_played(20000);
        entries.push_back({&swarm, swarm::scheduler::ActivityMode::play, swarm::evolution::evaluate(swarm)});
    }

    swarm::scheduler::TableManager manager;
    const auto plan = manager.build_plan(entries);
    require(plan.tables.size() == 2, "seventeen players yield two full tables");
    require(plan.tables[0].swarm_ids.size() == 8 && plan.tables[1].swarm_ids.size() == 8, "full tables are filled before leftovers are deferred");
    require(plan.deferred_swarm_ids.size() == 1, "single leftover swarm is gracefully deferred instead of solo-seated");

    std::vector<swarm::scheduler::SchedulingPassEntry> short_entries;
    short_entries.reserve(10);
    for (int i = 0; i < 10; ++i) {
        short_entries.push_back(entries[i]);
    }
    const auto short_plan = manager.build_plan(short_entries);
    require(short_plan.tables.size() == 2, "ten players produce one full table and one short table");
    require(short_plan.tables[0].swarm_ids.size() == 8, "first short-plan table stays full");
    require(short_plan.tables[1].swarm_ids.size() == 2, "leftover policy creates a short table when at least two swarms remain");
}

void test_table_manager_uses_activity_and_lifecycle_for_eligibility() {
    swarm::util::Rng rng(999);
    auto playing = make_swarm(2, 3, rng);
    auto resting = make_swarm(2, 3, rng);
    auto dead = make_swarm(2, 3, rng);

    playing.set_hands_played(20000);
    resting.set_hands_played(20000);
    dead.set_hands_played(100000);

    swarm::scheduler::TableManager manager;
    const auto plan = manager.build_plan({
        {&playing, swarm::scheduler::ActivityMode::play, swarm::evolution::evaluate(playing)},
        {&resting, swarm::scheduler::ActivityMode::active_rest, swarm::evolution::evaluate(resting)},
        {&dead, swarm::scheduler::ActivityMode::play, swarm::evolution::evaluate(dead)}});

    require(plan.tables.empty(), "insufficient eligible play-mode swarms produce no table");
    require(plan.deferred_swarm_ids.size() == 1 && plan.deferred_swarm_ids[0] == playing.id(), "only living play-mode swarms remain eligible but can still be deferred");
}

void test_chip_manager_supports_external_burns() {
    swarm::util::Rng rng(20260313);
    auto swarm = make_swarm(2, 2, rng);
    swarm.remove_bankroll(swarm.bankroll());

    swarm::economy::ChipManager manager;
    manager.inject(swarm, 5000);
    manager.burn_external(7);

    require(swarm.bankroll() == 5000, "external burn does not mutate a swarm stack directly");
    require(manager.chips_burned() == 7, "external burn increases burned tally");
    require(manager.chips_in_play() == 4993, "external burn reduces chips in play");
    require(manager.invariants_hold(), "external burn preserves chip invariant");
}

void test_simulation_population_initialization_is_accounted_and_deterministic() {
    swarm::sim::PopulationConfig config;
    config.swarm_count = 10;
    config.seed = 424242;

    auto first = swarm::sim::Population::create_default(config);
    auto second = swarm::sim::Population::create_default(config);

    require(first.swarms().size() == 10, "population creates requested swarm count");
    require(first.chip_manager().chips_in_play() == static_cast<std::int64_t>(10 * swarm::core::Swarm::starting_bankroll), "population bankroll is injected into chip manager");
    require(first.chip_manager().invariants_hold(), "population chip invariants hold after initialization");
    require(first.swarms().front().total_agents() == second.swarms().front().total_agents(), "population initialization is deterministic for first swarm size");
    require(first.swarms()[3].hands_played() == second.swarms()[3].hands_played(), "population initialization is deterministic for lifecycle seeding");
}

void test_population_prunes_dead_swarms_from_active_container() {
    swarm::sim::PopulationConfig config;
    config.swarm_count = 6;
    config.seed = 7777;

    auto population = swarm::sim::Population::create_default(config);
    const auto victim_id = population.swarms().front().id();
    population.swarms().front().mark_dead();
    population.prune_dead();

    require(population.swarms().size() == 5, "dead swarm removed from active container");
    require(population.dead_swarms().size() == 1, "dead swarm retained in graveyard bookkeeping");
    require(population.find_swarm(victim_id) != nullptr, "find_swarm can still locate segregated dead swarm for bookkeeping lookups");
    require(!population.find_swarm(victim_id)->alive(), "segregated dead swarm remains dead");
}

void test_simulation_step_preserves_chip_accounting_and_advances_state() {
    swarm::sim::SimulationConfig config;
    config.population.seed = 9090;
    config.population.swarm_count = 12;
    config.hands_per_table_block = 3;

    swarm::sim::Simulation simulation(config);
    const auto chips_before = simulation.population().chip_manager().chips_in_play();
    const auto swarms_before = simulation.population().swarms().size();

    const auto result = simulation.step_block();
    const auto& latest = simulation.statistics().latest();

    require(result.time.hand_block == 1, "simulation step advances one hand block");
    require(result.chip_accounting_ok, "simulation step preserves chip accounting");
    require(latest.total_bankroll == chips_before, "total bankroll matches chips in play after one block");
    require(simulation.population().chip_manager().chips_in_play() == chips_before, "chip manager chips in play remain stable without burns/injections dominating first step");
    require(latest.total_swarms >= swarms_before, "simulation maintains or grows swarm population after reproduction pass");
    require(latest.playing_swarms + latest.resting_swarms + latest.sleeping_swarms == latest.total_swarms, "every swarm has an activity bucket");
}

void test_simulation_tracks_rake_as_external_burn() {
    swarm::sim::SimulationConfig config;
    config.population.seed = 654321;
    config.population.swarm_count = 24;
    config.hands_per_table_block = 3;
    config.rake_per_hand = 5;

    swarm::sim::Simulation simulation(config);
    simulation.run_blocks(20);

    require(simulation.population().chip_manager().chips_burned() > 0, "integrated simulation burns rake through chip manager when tables run");
    require(simulation.statistics().latest().chip_accounting_ok, "integrated simulation keeps accounting consistent with nonzero rake");
}

void test_simulation_run_blocks_produces_statistics_history() {
    swarm::sim::SimulationConfig config;
    config.population.seed = 123456;
    config.population.swarm_count = 14;
    config.hands_per_table_block = 2;

    swarm::sim::Simulation simulation(config);
    simulation.run_blocks(4);

    require(simulation.statistics().history().size() == 4, "simulation records one statistics snapshot per block");
    require(simulation.statistics().latest().chip_accounting_ok, "latest simulation snapshot reports healthy accounting");
    require(!simulation.latest_statistics_json().empty(), "simulation exports structured statistics");
}

} // namespace

int main() {
    test_hand_ordering();
    test_tiebreakers();
    test_side_pot_eligibility_uneven_all_in();
    test_betting_round_allows_reraises_and_reopens_action();
    test_short_stack_calls_all_in_instead_of_forced_fold();
    test_rake_by_street_and_chop_logic();
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
    test_social_info_exchange_truthful_vs_deceptive();
    test_social_skepticism_changes_interpretation();
    test_alpha_led_governance_uses_advisor_signal();
    test_alpha_led_remains_distinct_from_democratic();
    test_rng_seed_reproducibility_and_ranges();
    test_default_policy_alignment();
    test_swarm_birth_distribution_sum_of_two_dice();
    test_mate_selection_respects_eligibility_and_parity();
    test_time_manager_tracks_ticks_and_hand_blocks();
    test_scheduler_enforces_minimum_sleep_ratio();
    test_table_manager_prevents_double_booking_and_fills_full_table();
    test_table_manager_handles_leftovers_gracefully();
    test_table_manager_uses_activity_and_lifecycle_for_eligibility();
    test_chip_manager_supports_external_burns();
    test_simulation_population_initialization_is_accounted_and_deterministic();
    test_population_prunes_dead_swarms_from_active_container();
    test_simulation_step_preserves_chip_accounting_and_advances_state();
    test_simulation_tracks_rake_as_external_burn();
    test_simulation_run_blocks_produces_statistics_history();
    std::cout << "PASS: test_poker\n";
    return 0;
}
