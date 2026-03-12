# Swarm Poker Ecosystem - Status

## Stage A / Phase 4

Status: **verified milestone complete** for the first lifecycle/evolution substrate pass. Phase 1 poker still builds and passes, Phase 2 core swarm/entity structure is still in place, Phase 3 economy remains intact, and Phase 4 now adds explicit lifecycle evaluation, constrained reproduction, and simple but real mutation/crossover plumbing.

## What Phase 4 now covers

This milestone now implements an honest evolution layer under `src/evolution`:
- **Lifecycle model** via `LifecyclePolicy` and `evaluate()`:
  - **youth** for swarms below **10,000 hands**
  - **maturity** from **10,000** up to **90,000 hands**
  - **old age** from **90,000** up to **100,000 hands**
  - **death** when a swarm reaches **100,000 hands** or bankroll drops to **0 or below**
- **Reproduction gating**:
  - requires both parents to be **alive**
  - requires both parents to be in **maturity**
  - enforces a per-parent **cooldown** in hands between births
  - enforces a per-parent **offspring cap**
- **Pairing rule** via `pairing_parity_allowed()`:
  - only **even-sized swarm × odd-sized swarm** pairings are allowed
- **Offspring construction** via `reproduce()`:
  - each parent contributes **one chromosome**
  - offspring receives **two chromosomes total**
  - offspring starts with bankroll **5000**
  - parent reproduction timestamps/counters are updated on success
- **Mutation layer** via `mutate_genome()` and `mutate_chromosome()`:
  - point mutation hooks for genome index vectors
  - meta mutation hooks for scalar genes like alpha/risk/confidence
  - structural mutation hooks for hidden-unit count, ocean radius, and chromosome agent count
- **Crossover layer** via `maybe_crossover()`:
  - low-probability recombination between the two inherited chromosome blueprints before offspring instantiation

## What changed in this milestone

- added `src/evolution/lifecycle.h/.cpp`
- added `src/evolution/reproduction.h/.cpp`
- added `src/evolution/mutation.h/.cpp`
- added `src/evolution/crossover.h/.cpp`
- updated `src/core/swarm.h/.cpp` so swarms now track:
  - hands played
  - last reproduction hand
  - offspring count
- updated `src/main.cpp` demo output to show lifecycle phase and reproduction readiness
- updated `test/test_poker.cpp` with lifecycle/evolution coverage
- updated `CMakeLists.txt` to compile the evolution sources

## Invariants now enforced

The implementation now enforces these lifecycle/evolution rules in code/tests:
- swarms below **10,000 hands** are in youth and cannot reproduce
- swarms from **10,000** to below **90,000 hands** are mature and may reproduce only if cooldown/cap checks pass
- swarms from **90,000** to below **100,000 hands** are old and cannot reproduce
- swarms at **100,000 hands** are dead
- swarms with bankroll **<= 0** are treated as dead by lifecycle evaluation
- reproduction rejects pairings unless one parent swarm size is even and the other is odd
- reproduction rejects invalid parents that are dead, too young, too old, still on cooldown, or over cap
- successful offspring always have two realized chromosomes, valid chromosome sizes, valid total swarm size, and the standard starting bankroll

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp -o test_poker.exe`

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
  - economy accounting identity under injections, transfers, and inheritance
  - lifecycle phase transitions
  - even×odd reproduction pairing enforcement
  - invalid parent rejection for youth, cooldown, and dead-bankroll cases
  - structurally valid offspring creation with correct initial state

### Runtime sanity run

- `swarm_poker_sim.exe --hands 10 --quiet`
- The executable now prints one sample swarm decision plus lifecycle phase/reproduction readiness, then the poker summary.

## Honest current limitations

Still intentionally deferred after this first lifecycle/evolution pass:
- there is **no population scheduler yet** that automatically advances all swarms through hands, pairing, birth, and death
- there is **no mating preference or social layer yet** beyond the explicit even×odd pairing rule
- reproduction currently returns a newly created offspring swarm, but does **not** register it into a persistent population graph or inheritance family tree
- crossover is intentionally simple and blueprint-level rather than a richer per-locus biological model
- mutation hooks are real and testable, but still modest; they do not yet model richer gene semantics or multi-step developmental constraints
- lifecycle death evaluation is available and usable now, but it is **not yet wired into a full world loop** that continuously applies mortality/inheritance/reproduction across generations
- poker outcomes are still not coupled to swarm lifecycle progression beyond the shared bankroll concept and explicit hand counters stored on each swarm

## Readiness

**Yes, this is now a truthful start to Phase 4.** The project now has:
1. explicit lifecycle phase evaluation
2. mature-only reproduction with cooldown and offspring-cap gates
3. enforced even×odd swarm pairing
4. offspring construction from one chromosome per parent
5. simple real mutation/crossover hooks for later evolution work
6. tests proving lifecycle transitions and reproduction constraints

Recommended next step:
1. add a persistent population manager or scheduler that advances hands and attempts pairings
2. connect lifecycle death with inheritance/family bookkeeping automatically
3. then layer social/mating preferences and longer-run generational simulation onto this substrate
