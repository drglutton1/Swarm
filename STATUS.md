# Swarm Poker Ecosystem - Status

## Stage A / Phase 1

Status: implemented poker-engine foundation for a simple no-betting simulation loop with 8 random bots, hand evaluation, chip accounting, test harness, and benchmark mode.

## What exists

- Core card/deck model (`src/poker/card.h`, `src/poker/deck.h`)
- Best-of-seven hand evaluation with category/tiebreak comparison (`src/poker/hand_evaluator.h`)
- Pot/rake accounting (`src/poker/pot_manager.h`)
- Simple table simulator for 8 random bots over repeated hands with verbose logging (`src/poker/table.h`)
- Deterministic RNG wrapper (`src/util/rng.h`)
- CLI entry point with normal and benchmark modes (`src/main.cpp`)
- Basic tests covering ranking order, wheel straight detection, and chip conservation over 5000 hands (`test/test_poker.cpp`)
- Build metadata and baseline config (`CMakeLists.txt`, `config/default.toml`)

## Current phase notes

- Phase 1 target met for engine foundation, correct broad hand-ranking categories, chip conservation, and a benchmark harness.
- The simulation intentionally keeps betting logic simple: every player posts an ante, then randomly folds or goes to showdown.
- This is a foundation for later Stage A phases where real action rounds, blinds, side pots, and strategy evolution should be layered in.

## Performance

- Last verified build: `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -pthread -Isrc src/main.cpp -o swarm_poker_sim`
- Last verified tests: `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -pthread -Isrc test/test_poker.cpp -o test_poker && .\\test_poker`
- Last benchmark: `swarm_poker_sim.exe --benchmark 100000` -> 100,000 hands in ~2566.31 ms on this machine with chip conservation preserved.

## Known limitations / honest gaps

- No blinds, betting streets, raises, all-ins, side pots, or positional logic yet.
- Pot manager supports a single shared pot and rake; it is not yet a full side-pot engine.
- Random bots currently make only a preflop continue/fold decision; no deception or social signaling yet.
- `config/default.toml` documents defaults but is not parsed by the executable yet.
- Logging is human-readable text only; structured analyzable output (CSV/JSON) should be added in a later phase.
- Benchmark mode uses a deeper stack and zero rake so throughput can be measured without bankroll exhaustion distorting long runs.
