#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/agent.h"
#include "core/chromosome.h"
#include "core/genome.h"
#include "core/ocean.h"
#include "core/swarm.h"
#include "poker/table.h"
#include "util/rng.h"

namespace {

void print_usage() {
    std::cout << "Usage:\n"
              << "  swarm_poker_sim [--hands N] [--seed N] [--benchmark N] [--quiet]\n";
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

swarm::core::Agent make_agent(swarm::util::Rng& rng, std::uint32_t ocean_size, std::size_t state_size) {
    constexpr std::size_t parameter_count = 64;
    auto genome = swarm::core::Genome::random(parameter_count, ocean_size, rng);
    return swarm::core::Agent(std::move(genome), swarm::core::DecoderConfig{state_size, 4, 3});
}

} // namespace

int main(int argc, char** argv) {
    try {
        int hands = 10;
        bool verbose = true;
        bool benchmark = false;
        int benchmark_hands = 100000;
        std::uint64_t seed = 1337;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--hands" && i + 1 < argc) {
                hands = std::stoi(argv[++i]);
            } else if (arg == "--seed" && i + 1 < argc) {
                seed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
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

        swarm::util::Rng rng(seed + 99);
        swarm::core::Ocean ocean(256);
        ocean.initialize_uniform(-1.0f, 1.0f, rng);
        ocean.decay(0.995f);

        const std::size_t state_size = 6;
        std::vector<swarm::core::Agent> odd_agents;
        std::vector<swarm::core::Agent> even_agents;
        for (int i = 0; i < 3; ++i) {
            odd_agents.push_back(make_agent(rng, static_cast<std::uint32_t>(ocean.size()), state_size));
            even_agents.push_back(make_agent(rng, static_cast<std::uint32_t>(ocean.size()), state_size));
        }

        swarm::core::Swarm swarm(
            swarm::core::Chromosome(std::move(odd_agents)),
            swarm::core::Chromosome(std::move(even_agents)));

        const swarm::core::PokerStateVector sample_state{{0.65f, 0.30f, 0.55f, 0.10f, 0.75f, 0.40f}, 5.0f, 10.0f, 25.0f, 100.0f};
        const auto decision = swarm.decide(ocean, sample_state, 1);

        std::cout << "Swarm demo: governance="
                  << (swarm.governance_for_turn(1) == swarm::core::GovernanceMode::alpha_led ? "alpha-led" : "democratic")
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
