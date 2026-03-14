# Swarm Poker Ecosystem - Status

## Stage A / Phase 6

Status: **verified Phase 6 scheduler substrate implemented and locally tested after the clean restart.**

This repo now contains:
- the earlier curator corrective poker/spec pass,
- the pre-Phase-6 governance/RNG hardening pass, and
- a first real scheduler substrate for time progression, activity scheduling, and table formation.

## Phase 6 implemented

### 1) Time management

Added `src/scheduler/time_manager.h/.cpp`.

What it now supports:
- explicit scheduler-facing `tick`, `hand_block`, `hands_elapsed`, and `tick_in_block`
- configurable `hands_per_block`
- configurable `ticks_per_block`
- advancing by ticks or by hand blocks
- deterministic progression suitable for driving future scheduling passes

This is intentionally infrastructure, not a full world-clock or event engine.

### 2) Activity scheduling

Added `src/scheduler/activity_scheduler.h/.cpp`.

What it now supports:
- three activity modes:
  - `play`
  - `active_rest`
  - `sleep`
- a persistent `ActivityState` with tracked play/rest/sleep tick counts
- `ActivityPreferences` designed to be evolvable later instead of hard-coding a final behavior model
- preference derivation from existing swarm traits (`confidence`, `risk`, `honesty`, `skepticism`)
- lifecycle-aware scheduling hooks using existing `evolution::evaluate(...)`
- minimum sleep enforcement via a cycle-based rule that guarantees at least **30% sleep** across each scheduler cycle

Truthful scope note:
- this is a substrate for scheduling eligibility and rhythm
- it is **not** a full fatigue model, circadian system, or Phase 7 behavioral simulator

### 3) Table management

Added `src/scheduler/table_manager.h/.cpp`.

What it now supports:
- builds a scheduling pass from swarm entries containing:
  - swarm pointer
  - current activity mode
  - lifecycle state
- only swarms that are both:
  - in `play` mode, and
  - alive / bankroll-positive by lifecycle/economy state
  are considered table-eligible
- fills tables with a target size of **8 swarms** when possible
- guarantees **no double-booking** in a single scheduling pass
- sorts eligible swarms deterministically (bankroll desc, then id) before seating

### Leftovers policy

Implemented honest, simple leftovers handling:
- fill all possible 8-swarm tables first
- if **2 to 7** eligible swarms remain, create one short table
- if **1** eligible swarm remains, do **not** fabricate a solo table; defer that swarm to the next pass

That policy is graceful without pretending a richer matchmaking layer already exists.

## Files created / changed in Phase 6

### New files
- `src/scheduler/time_manager.h`
- `src/scheduler/time_manager.cpp`
- `src/scheduler/activity_scheduler.h`
- `src/scheduler/activity_scheduler.cpp`
- `src/scheduler/table_manager.h`
- `src/scheduler/table_manager.cpp`

### Updated files
- `test/test_poker.cpp`
  - added scheduler coverage for:
    - time/block progression
    - minimum 30% sleep enforcement
    - no double-booking
    - 8-seat fill correctness
    - leftovers policy
    - lifecycle/activity-based table eligibility
- `STATUS.md`

## Verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp src/scheduler/time_manager.cpp src/scheduler/activity_scheduler.cpp src/scheduler/table_manager.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp src/scheduler/time_manager.cpp src/scheduler/activity_scheduler.cpp src/scheduler/table_manager.cpp -o test_poker.exe`

### Test / run results

- `test_poker.exe`
  - Result: **PASS**
- `swarm_poker_sim.exe --hands 10 --quiet`
  - Result: **runs successfully**
  - Sample verified output included `hands=10`, `rake=30`, `chips_ok=true`

## Honest current limitations

Phase 6 now provides a real scheduler substrate, but it is still intentionally limited:
- activity selection is still a compact heuristic policy, not a learned or fully evolved behavior system
- scheduler state is not yet threaded through the main simulation loop as a full multi-table world orchestrator
- table management currently outputs assignments/deferred swarms; it does not yet run parallel table economies or per-table hand execution
- leftovers policy is intentionally simple and honest, not a sophisticated balancing or waitlist optimizer
- no Phase 7 social/meta-game simulation was added here

## Readiness

**Yes — this milestone is strong enough that Phase 7 can begin.**

Reason:
- time progression substrate exists
- play/rest/sleep scheduling exists with enforced minimum sleep
- table assignment exists with truthful eligibility rules and deterministic leftovers handling
- regression coverage for the required scheduler behaviors passes locally

Honest caveat:
- Phase 7 should build on this scheduler rather than assume it already simulates richer social coordination, travel, fatigue, or full ecosystem orchestration.
