#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/ocean.h"
#include "core/swarm.h"
#include "poker/table.h"
#include "util/rng.h"
#include "evolution/lifecycle.h"
#include "social/face.h"
#include "social/info_exchange.h"
#include "sim/simulation.h"

namespace {

void print_usage() {
    std::cout << "Usage:\n"
              << "  swarm_poker_sim [--hands N] [--seed N] [--benchmark N] [--quiet]\n"
              << "  swarm_poker_sim --simulate-blocks N [--population N] [--ocean-size N] [--seed N] [--activity-quota-play R] [--activity-quota-rest R] [--activity-quota-sleep R] [--quiet]\n";
}

const char* action_name(swarm::core::ActionType action) {
    switch (action) {
        case swarm::core::ActionType::fold:
            return "fold";
        case swarm::core::ActionType::check_call:
            return "check/call";
        case swarm::core::ActionType::raise:
            return "raise";
    }
    return "unknown";
}

const char* phase_name(swarm::evolution::LifePhase phase) {
    switch (phase) {
        case swarm::evolution::LifePhase::youth:
            return "youth";
        case swarm::evolution::LifePhase::maturity:
            return "maturity";
        case swarm::evolution::LifePhase::old_age:
            return "old_age";
        case swarm::evolution::LifePhase::dead:
            return "dead";
    }
    return "unknown";
}

} // namespace

int main(int argc, char** argv) {
    try {
        int hands = 10;
        bool verbose = true;
        bool benchmark = false;
        int benchmark_hands = 100000;
        int simulate_blocks = 0;
        std::size_t population_size = 12;
        std::uint64_t seed = 1337;
        std::size_t ocean_size = 512;
        float activity_quota_play = 0.0f;
        float activity_quota_rest = 0.0f;
        float activity_quota_sleep = 0.0f;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--hands" && i + 1 < argc) {
                hands = std::stoi(argv[++i]);
            } else if (arg == "--simulate-blocks" && i + 1 < argc) {
                simulate_blocks = std::stoi(argv[++i]);
            } else if (arg == "--population" && i + 1 < argc) {
                population_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            } else if (arg == "--seed" && i + 1 < argc) {
                seed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
            } else if (arg == "--ocean-size" && i + 1 < argc) {
                ocean_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            } else if (arg == "--activity-quota-play" && i + 1 < argc) {
                activity_quota_play = std::stof(argv[++i]);
            } else if (arg == "--activity-quota-rest" && i + 1 < argc) {
                activity_quota_rest = std::stof(argv[++i]);
            } else if (arg == "--activity-quota-sleep" && i + 1 < argc) {
                activity_quota_sleep = std::stof(argv[++i]);
            } else if (arg == "--benchmark" && i + 1 < argc) {
                benchmark = true;
                benchmark_hands = std::stoi(argv[++i]);
                verbose = false;
            } else if (arg == "--quiet") {
                verbose = false;
            } else if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            } else {
                throw std::runtime_error("unknown argument: " + arg);
            }
        }

        if (simulate_blocks > 0) {
            swarm::sim::SimulationConfig config;
            config.population.seed = seed;
            config.population.swarm_count = population_size;
            config.population.ocean_size = ocean_size;
            config.activity.fixed_play_ratio = activity_quota_play;
            config.activity.fixed_rest_ratio = activity_quota_rest;
            config.activity.fixed_sleep_ratio = activity_quota_sleep;
            swarm::sim::Simulation simulation(config);
            simulation.run_blocks(static_cast<std::size_t>(simulate_blocks));
            std::cout << simulation.latest_statistics_json();
            return simulation.statistics().latest().chip_accounting_ok ? 0 : 2;
        }

        swarm::util::Rng rng(seed + 99);
        swarm::core::Ocean ocean(512);
        ocean.initialize_uniform(-1.0f, 1.0f, rng);
        ocean.diffuse(0.25f);
        ocean.decay(0.995f);
        ocean.deposit(17, 0.2f);

        const std::size_t state_size = 6;
        swarm::core::Swarm swarm = swarm::core::Swarm::random(static_cast<std::uint32_t>(ocean.size()), state_size, rng);

        const swarm::core::PokerStateVector sample_state{{0.65f, 0.30f, 0.55f, 0.10f, 0.75f, 0.40f}, 5.0f, 5.0f, 25.0f, 100.0f};
        swarm.set_hands_played(15000);
        const auto decision = swarm.decide(ocean, sample_state, 1);
        const auto lifecycle = swarm::evolution::evaluate(swarm);
        const auto face = swarm::social::make_public_face(swarm);
        const auto private_bankroll = swarm::social::request_private_info(swarm, swarm, swarm::social::PrivateInfoField::bankroll, rng);

        std::cout << "Swarm demo: id=" << swarm.id()
                  << ", agents=" << swarm.total_agents()
                  << ", governance=" << (swarm.governance_mode() == swarm::core::GovernanceMode::alpha_led ? "alpha-led" : "democratic")
                  << ", bankroll=" << swarm.bankroll()
                  << ", phase=" << phase_name(lifecycle.phase)
                  << ", reproduction_ready=" << (lifecycle.reproduction_window_open ? "true" : "false")
                  << ", chromosomes=" << swarm.first_chromosome().size() << '+' << swarm.second_chromosome().size()
                  << ", public_honesty=" << face.self_reported_honesty
                  << ", private_bankroll_answer=" << (private_bankroll.refused ? -1.0 : private_bankroll.reported_value)
                  << ", action=" << action_name(decision.action)
                  << ", fold=" << decision.action_scores[0]
                  << ", check_call=" << decision.action_scores[1]
                  << ", raise=" << decision.action_scores[2]
                  << ", raise_amount=" << decision.raise_amount
                  << ", confidence=" << decision.confidence << '\n';

        const std::int64_t starting_stack = benchmark ? 1000000 : 1000;
        const std::int64_t rake_per_hand = benchmark ? 0 : 1;
        swarm::poker::Table table(8, starting_stack, 5, 5, rake_per_hand, seed);
        const auto summary = table.run(benchmark ? benchmark_hands : hands, verbose, std::cout);

        std::cout << "Summary: hands=" << summary.hands_played
                  << ", players=" << summary.players
                  << ", initial_chips=" << summary.initial_chips
                  << ", final_chips=" << summary.final_chips
                  << ", rake=" << summary.total_rake
                  << ", elapsed_ms=" << summary.elapsed_ms
                  << ", chips_ok=" << (table.chip_conservation_holds() ? "true" : "false")
                  << '\n';

        std::cout << "Ending stacks:";
        for (std::size_t i = 0; i < summary.ending_stacks.size(); ++i) {
            std::cout << " Bot" << (i + 1) << '=' << summary.ending_stacks[i];
        }
        std::cout << '\n';

        return table.chip_conservation_holds() ? 0 : 2;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        print_usage();
        return 1;
    }
}
