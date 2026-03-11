# Swarm Poker Ecosystem - Status

## Stage A / Phase 2

Status: **verified milestone complete** for the first real core-entities pass. The repo now keeps the already-verified Phase 1 Hold'em layer intact and adds a buildable/tested Phase 2 foundation for shared parameters, genomes, decoders, agents, chromosomes, and swarms.

## What Phase 2 now covers

This pass adds an honest minimal data flow for swarm decision-making:
- **Ocean**: shared float parameter store with indexed reads/writes, uniform initialization, fill, and multiplicative decay.
- **Genome**: ordered `uint32_t` parameter index list into the ocean plus simple structure/meta genes (`hidden_units`, `action_count`, aggressiveness/confidence genes).
- **Decoder**: compact feed-forward decoder that consumes ocean-selected parameters through the genome and turns a poker state vector into three action scores (`fold`, `check/call`, `raise`), a bounded raise amount, and a bounded confidence value.
- **Agent**: owns a genome plus decoder config and produces a real decision from a supplied state vector.
- **Chromosome**: collection of agents with averaged intra-chromosome voting.
- **Swarm**: two chromosomes with turn-based governance:
  - odd-numbered turns -> **alpha-led** (first odd chromosome agent decides)
  - even-numbered turns -> **democratic** (both chromosomes are averaged together)

## Phase 1 behavior preserved

The existing poker engine remains in place and still provides:
- dealer/button rotation
- 5 / 5 blinds
- preflop / flop / turn / river flow
- showdown evaluation
- chip conservation tracking

## What changed in this milestone

- added `src/core/ocean.h/.cpp`
- added `src/core/genome.h/.cpp`
- added `src/core/decoder.h/.cpp`
- added `src/core/agent.h/.cpp`
- added `src/core/chromosome.h/.cpp`
- added `src/core/swarm.h/.cpp`
- updated `CMakeLists.txt` to build the new core sources as `swarm_core`
- updated `src/main.cpp` to print a simple swarm-decision demo before running the poker simulation
- expanded `test/test_poker.cpp` to verify Phase 1 behavior plus Phase 2 decision validity across 100 random swarms

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp -o test_poker.exe`

### Test command

- `test_poker.exe`
- Expected/verified scope:
  - poker evaluator ordering
  - poker tiebreak behavior
  - chip conservation
  - 5/5 blind logging
  - heads-up blind order
  - ocean initialization/decay sanity
  - 100 random swarms producing valid decisions with sane score/confidence/raise ranges

### Runtime sanity run

- `swarm_poker_sim.exe --hands 10 --quiet`
- The executable now prints one sample swarm decision and then the poker summary.

## Honest current limitations

Still intentionally simplified for this milestone:
- the decoder uses a very small hand-built feed-forward pass, not a rich neural architecture
- the poker state input is still a compact synthetic feature vector, not a full direct extraction from live table state
- no learning, mutation, crossover, selection pressure, or persistence yet
- no memory, deception, social coalition logic, or evolutionary governance yet
- no side-pot engine or true all-in continuation yet in the poker layer
- no analytics/dashboard layer yet

## Readiness

This repo is now in a truthful state to proceed to the next Phase 2/3 steps:
1. connect the state vector to richer real table features
2. add mutation/crossover and swarm population management
3. integrate swarm decisions into live betting policy
4. add structured analytics output for training/evaluation
