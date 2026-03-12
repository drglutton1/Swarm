# Swarm Poker Ecosystem - Status

## Stage A / Phase 5

Status: **verified milestone complete** for the first social-layer substrate pass. Phases 1-4 still build and pass, and Phase 5 now adds a real but intentionally modest social layer: public faces, private information exchange with truth/lie/refuse behavior, skepticism-weighted interpretation, and partner selection that respects lifecycle eligibility plus the even×odd reproduction constraint.

## What Phase 5 now covers

This milestone adds `src/social` and wires it into the existing swarm/evolution substrate:

- **Public face / social profile** via `face.h/.cpp`:
  - every swarm now exposes a stable **swarm id**
  - public face includes:
    - `swarm_id`
    - `hands_played`
    - `size`
    - `offspring_count`
    - self-reported `risk`, `confidence`, and `honesty`
- **Private information exchange** via `info_exchange.h/.cpp`:
  - one swarm can request a private field from another
  - supported private fields currently include:
    - bankroll
    - reproductive readiness
    - honesty gene
    - skepticism gene
    - confidence gene
  - responder behavior uses **honesty_gene (0..1)**:
    - high honesty tends toward truthful answers
    - low honesty can produce deceptive answers
    - sufficiently low honesty can also refuse to answer
  - receiver interpretation uses **skepticism_gene (0..1)**:
    - low skepticism trusts the reported answer more strongly
    - high skepticism discounts the reported answer more heavily
- **Mate selection** via `mate_selection.h/.cpp`:
  - evaluates candidate partners from:
    - public face data
    - optional interpreted exchanged info
  - rejects partners that are:
    - self
    - parity-incompatible
    - not currently reproduction-eligible under lifecycle rules
  - scoring modestly prefers candidates with:
    - reproduction readiness
    - stronger believed bankroll
    - better believed honesty
    - fewer prior offspring
- **Core integration**:
  - `Genome` now carries `honesty_gene` and `skepticism_gene`
  - mutation now mutates those social genes too
  - `Swarm` now has a stable `id()` plus averaged social/economic gene accessors
  - `main.cpp` demo output now shows a sample public honesty signal and one private-info answer

## What changed in this milestone

- added `src/social/face.h/.cpp`
- added `src/social/info_exchange.h/.cpp`
- added `src/social/mate_selection.h/.cpp`
- updated `src/core/genome.h/.cpp`:
  - added `honesty_gene`
  - added `skepticism_gene`
- updated `src/evolution/mutation.cpp` so those social genes mutate
- updated `src/core/swarm.h/.cpp`:
  - added stable swarm ids
  - added averaged gene accessors for risk/confidence/honesty/skepticism
- updated `src/main.cpp` demo output to exercise the new social layer
- updated `test/test_poker.cpp` with Phase 5 verification
- updated `CMakeLists.txt` to compile the social sources

## Invariants now enforced

The implementation now enforces these social-layer rules in code/tests:
- a swarm has a stable public-facing id for cross-swarm social references
- public face exposes selected profile fields without exposing the full internal swarm state
- private info requests can result in a truthful answer, a deceptive answer, or a refusal
- honesty gene materially affects answer behavior
- skepticism gene materially affects how a received answer is interpreted
- partner selection only considers candidates that pass lifecycle eligibility checks and the even×odd pairing rule from Phase 4

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o test_poker.exe`

### Test command

- `test_poker.exe`
- Result: **PASS**
- Verified scope now includes:
  - prior poker/core/economy/evolution coverage from earlier phases
  - truthful vs deceptive private responses
  - skepticism-weighted interpretation
  - mate selection respecting eligibility and parity constraints

### Runtime sanity run

- `swarm_poker_sim.exe --hands 10 --quiet`
- Result: executable runs successfully and prints a sample socialized swarm demo plus the poker summary.

## Honest current limitations

Still intentionally deferred after this first social pass:
- there is **no world scheduler** yet that drives repeated encounters, gossip, courtship, or longitudinal reputation
- public face is currently a **simple snapshot**, not an evolving social reputation system
- information exchange is currently **request/response only**; there is no memory store, rumor propagation, or multi-hop communication layer yet
- skepticism weighting currently discounts answers against a neutral baseline derived from the known truth in the response object; this is sufficient for substrate/tests, but future integration should move toward receiver-side priors and memory rather than omniscient test scaffolding
- mate selection is a **single-pass scoring helper**, not a full population matchmaking loop
- partner choice does not yet feed directly into an automated reproduction scheduler; it is ready for that next layer but does not implement it itself
- deception strategy is still coarse; dishonest swarms fabricate plausible scalar values, but there is no richer narrative or context-aware lying model yet

## Readiness

**Yes, this is now a truthful start to Phase 5.** The project now has:
1. public swarm faces with stable ids and selected self-reported traits
2. private information requests with truth/lie/refuse behavior driven by honesty gene
3. receiver-side trust discounting driven by skepticism gene
4. partner selection over public plus exchanged info
5. explicit compatibility with lifecycle eligibility and even×odd reproduction parity
6. tests proving the core social mechanics actually work

Recommended next step:
1. add a persistent social memory/reputation store per swarm
2. connect mate selection into a real population scheduler/world loop
3. then layer repeated encounters, alliances, betrayal, and analyzable social dynamics onto this substrate
