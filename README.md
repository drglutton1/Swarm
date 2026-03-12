# Swarm NLTH Ecosystem

A staged C++17 implementation of the Swarm Poker Ecosystem: agent swarms playing No-Limit Texas Hold'em inside an evolving economic, reproductive, and social simulation.

## Current status

Implemented and committed:
- Phase 1: Poker layer
- Phase 2: Core entities
- Phase 3: Economy
- Phase 4: Lifecycle & evolution
- Phase 5: Social layer

Not yet implemented:
- Phase 6: Scheduler
- Phase 7: Full simulation loop
- Phase 8: Scale/overnight/checkpointing/analysis

## Authoritative project rules currently fixed

- Language: C++17
- Blinds: SB=5 / BB=5
- Swarm size: 4-18 agents total
- Reproduction parity: even-sized swarm x odd-sized swarm only
- Starting bankroll: 5000 chips

## Build

Using MinGW g++ on Windows:

```powershell
C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o swarm_poker_sim.exe
```

## Test

```powershell
C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o test_poker.exe
.\test_poker.exe
```

## Notes for reviewers

This repository is currently at the "verified substrate" stage:
- the first five layers exist and are testable,
- but the full scheduler/world loop and large-scale simulation are not built yet.

The biggest review targets right now are:
- poker fidelity gaps (side pots / all-in handling / betting realism)
- architectural soundness of swarm/core/economy/evolution/social integration
- readiness for scheduler-driven world execution

## Project status document

See `STATUS.md` for the latest milestone summary and known limitations.
