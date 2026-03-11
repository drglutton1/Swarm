#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "poker/table.h"

namespace {

void print_usage() {
    std::cout << "Usage:\n"
              << "  swarm_poker_sim [--hands N] [--seed N] [--benchmark N] [--quiet]\n";
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
