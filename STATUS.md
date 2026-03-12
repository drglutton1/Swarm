# Swarm Poker Ecosystem - Status

## Stage A / Phase 3

Status: **verified milestone complete** for the first economy substrate pass. Phase 1 poker remains intact, Phase 2 swarm/core structure remains intact, and Phase 3 now introduces real chip accounting, swarm bankroll state, neutral inter-swarm transfers, and inheritance rules tied to swarm death.

## What Phase 3 now covers

This milestone now implements an honest economy layer under `src/economy`:
- **Chip accounting framework** via `ChipManager`:
  - tracks **chips injected**
  - tracks **chips burned**
  - tracks **chips in play**
  - enforces the invariant **chips_in_play + chips_burned = chips_injected**
- **Starting bankroll model**:
  - every new random swarm now starts with **5000 chips**
- **Transfers** via `TransferService`:
  - validates source/destination are different swarms
  - rejects non-positive transfers
  - rejects transfers that exceed source bankroll
  - rejects transfers involving dead swarms
  - preserves global accounting neutrality
- **Inheritance** via `InheritanceService`:
  - on death with living offspring: **50% burned, 50% distributed across living offspring**
  - on death with zero living offspring: **100% burned**
  - dead or null offspring are filtered out
  - policy is structured behind an `InheritancePolicy` interface so future parent-donation rules can extend the same flow cleanly

## What changed in this milestone

- added `src/economy/chip_manager.h/.cpp`
- added `src/economy/transfer.h/.cpp`
- added `src/economy/inheritance.h/.cpp`
- updated `src/core/swarm.h/.cpp` so swarms now carry bankroll and alive/dead state
- updated `src/main.cpp` demo output to print swarm bankroll
- updated `test/test_poker.cpp` with economy scenario coverage
- updated `CMakeLists.txt` to build the new economy sources

## Invariants now enforced

The implementation now enforces these economy rules in code/tests:
- every random swarm starts with **5000 chips**
- chip injections increase both **injected** and **in play**
- burns decrease **in play** and increase **burned**
- transfers move chips between swarms without changing global totals
- inheritance only pays living offspring
- inheritance marks the deceased swarm dead after processing
- the global accounting identity **chips_in_play + chips_burned = chips_injected** holds across a mixed scenario of injections, transfers, and inheritance events

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp -o test_poker.exe`

### Test command

- `test_poker.exe`
- Verified scope:
  - poker evaluator ordering
  - poker tiebreak behavior
  - chip conservation in the poker table
  - 5/5 blind logging
  - heads-up blind order
  - ocean substrate behavior
  - swarm birth/governance invariants
  - 200 random valid swarm decisions
  - economy accounting identity under injections, transfers, partial-burn inheritance, and full-burn inheritance

### Runtime sanity run

- `swarm_poker_sim.exe --hands 10 --quiet`
- The executable prints one sample swarm decision, now including bankroll, and then the poker summary.

## Honest current limitations

Still intentionally deferred after this first economy pass:
- no automatic coupling yet between the poker table chip flow and swarm bankrolls
- no full lifecycle engine yet for reproduction, offspring creation, or population turnover
- inheritance currently works on explicitly supplied offspring references rather than a persistent family graph
- inheritance payouts are integer chip splits; when exact division is impossible, remainder chips are distributed deterministically to the earliest living offspring rather than introducing fractional chips
- the `InheritancePolicy` abstraction is in place, but parent-donation and richer lineage rules are not implemented yet
- no persistent simulation loop yet that advances swarms through play, reproduction, death, and wealth transfer over generations

## Readiness

**Yes, this is now a truthful start to Phase 3.** The project now has:
1. swarm-level bankroll state
2. explicit global chip accounting
3. validated neutral transfers between swarms
4. inheritance rules with burning and offspring distribution
5. tests proving the accounting identity under mixed economy events

Recommended next step:
1. connect swarm bankrolls to actual poker outcomes
2. introduce persistent population bookkeeping for parent/offspring relationships
3. then layer reproduction/death scheduling and evolutionary turnover onto this economy substrate
