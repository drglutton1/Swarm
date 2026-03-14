# Swarm Poker Ecosystem - Status

## Stage A / Phase 7

Status: **verified Phase 7 simulation substrate implemented and locally smoke-tested.**

This is the first real integration layer tying together:
- swarm/core state
- scheduler time + activity + table planning
- poker table execution
- economy accounting
- lifecycle evaluation + mortality/inheritance hooks
- social info exchange + mate selection hooks
- reproduction into a growing population
- structured simulation statistics output

It is **not** a Phase 8 scale-up run. It is a cautious, testable substrate for stepping the ecosystem forward in blocks and checking whether the plumbing stays sane.

## Phase 7 implemented

### 1) Population layer

Added `src/sim/population.h/.cpp`.

What it now supports:
- a deterministic simulation population with:
  - shared `Ocean`
  - `std::vector<Swarm>`
  - `ChipManager`
  - seeded RNG
  - per-swarm runtime/activity bookkeeping
  - offspring linkage tracking for later inheritance
- deterministic/testable initialization of a small-to-moderate swarm population
- initial bankroll injection through the economy layer so chip accounting starts honest instead of relying on invisible bootstrap chips
- seeded lifecycle staging across youth / maturity / old-age cohorts

Honest note:
- this is still an in-memory population substrate, not a persistence layer or distributed world model.

### 2) Integrated simulation loop

Added `src/sim/simulation.h/.cpp`.

What it now supports:
- stepping the world by **hand blocks**
- per-block ocean refresh
- per-swarm lifecycle evaluation
- per-swarm scheduler activity updates using the Phase 6 scheduler substrate
- table planning via `scheduler::TableManager`
- actual table execution for assigned swarms through the existing poker engine
- bankroll deltas written back into swarm state after poker play
- swarm hand counters updated from real table execution
- mortality pass + inheritance processing hooks
- social-information gathering + mate-selection based reproduction attempt pass
- population growth when reproduction succeeds

Important constraint:
- the integrated substrate currently uses **zero rake by default** inside the Phase 7 world loop so chip accounting remains exact. The first smoke run exposed that table rake was leaking outside the simulation economy accounting model. Rather than pretend that was solved, Phase 7 keeps rake off until a proper ecosystem sink/account is implemented.

### 3) Statistics / output

Added `src/sim/statistics.h/.cpp`.

What it now supports:
- block-by-block snapshots capturing:
  - total / alive / dead swarm counts
  - youth / mature / old-age counts
  - reproduction-ready counts
  - play / rest / sleep activity counts
  - tables formed
  - hands resolved
  - births and processed deaths
  - bankroll totals / min / max / average
  - chip-accounting health flag
- JSON-style structured output for analysis
- `swarm_poker_sim.exe --simulate-blocks N [--population N] [--seed N]`

### 4) Test coverage

Updated `test/test_poker.cpp` with Phase 7 coverage for:
- deterministic population initialization
- initial chip-accounting correctness in the population layer
- one integrated simulation block advancing time and preserving accounting
- multi-block statistics history generation

## Files created / changed in Phase 7

### New files
- `src/sim/population.h`
- `src/sim/population.cpp`
- `src/sim/statistics.h`
- `src/sim/statistics.cpp`
- `src/sim/simulation.h`
- `src/sim/simulation.cpp`

### Updated files
- `CMakeLists.txt`
- `src/main.cpp`
- `test/test_poker.cpp`
- `STATUS.md`

## Verification performed

### Build commands run

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp src/scheduler/time_manager.cpp src/scheduler/activity_scheduler.cpp src/scheduler/table_manager.cpp src/sim/population.cpp src/sim/statistics.cpp src/sim/simulation.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp src/scheduler/time_manager.cpp src/scheduler/activity_scheduler.cpp src/scheduler/table_manager.cpp src/sim/population.cpp src/sim/statistics.cpp src/sim/simulation.cpp -o test_poker.exe`

### Test result

- `test_poker.exe`
  - Result: **PASS**

### Modest integrated smoke run

Command run:
- `swarm_poker_sim.exe --simulate-blocks 10 --population 12 --seed 1337`

What actually happened:
- simulation ran successfully and emitted structured history
- first four blocks had all swarms sleeping under the current scheduler cadence
- blocks 5-10 formed **1 table per block**
- each active block resolved **30 seated-hand participations** (`6` swarms seated for `5` hands)
- bankrolls redistributed across participants while total bankroll remained exactly **60000**
- chip accounting stayed **true** for all 10 blocks after switching the integrated loop to zero-rake mode
- no births or processed deaths occurred during this short smoke run

That is a real integrated run, but still a modest one.

## Honest current limitations

Phase 7 is real, but still intentionally conservative:
- poker table execution is integrated as a substrate, not yet a swarm-brain-driven seat-by-seat decision bridge
- social/reproduction is wired in, but the short smoke run did not yet produce offspring; this path is present but not yet richly validated under longer runs
- mortality/inheritance plumbing exists, but the short local smoke run did not drive deep death cascades
- no external persistence, checkpointing, or resume support
- no large-scale benchmarking, overnight runs, or profiling yet
- no ecosystem-level rake sink/account yet, which is why the integrated loop currently defaults to `rake_per_hand = 0`
- scheduler behavior currently produces a very coarse cadence (sleep-heavy early blocks, then stable play blocks)

## Readiness assessment

**Yes — the project is now ready for a cautious first real simulation run, but not yet a serious long-duration or high-scale ecosystem campaign.**

What "ready" means here:
- the major subsystems are now actually threaded together
- local tests pass
- a modest integrated run completed
- bankroll accounting remained sane
- structured output exists for inspection

What it does **not** mean:
- not ready to claim emergent society results
- not ready to claim reproduction dynamics are proven
- not ready to claim economy sinks/sources beyond the current honest model
- not ready to skip additional smoke runs, parameter tuning, and instrumentation before any overnight run
