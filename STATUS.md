# Swarm Poker Ecosystem - Status

## Stage A / Phase 2

Status: **verified milestone complete** for a stronger spec-alignment pass. Phase 1 Hold'em remains intact, and Phase 2 now models swarm birth, chromosome composition, governance, genomes, and the ocean substrate in a way that is materially closer to the intended ecosystem instead of the earlier placeholder form.

## What Phase 2 now covers

This pass now implements an honest, more structured swarm core:
- **Ocean**: shared circular parameter substrate with indexed reads/writes, uniform initialization, decay, diffusion, local neighborhood averaging, local gradient reads, and direct deposits. It now behaves more like a shared field than a flat float array.
- **Genome**: ordered ocean index segments for **sensory**, **hidden**, **action**, and **modulation** bands, plus structure genes (`hidden_units`, `ocean_window_radius`, `chromosome_agent_count`) and behavioral genes (`alpha_bias`, `risk_gene`, `confidence_gene`).
- **Chromosome**: no longer just an arbitrary bucket of agents. Each chromosome now has a **blueprint genome** and spawns its agent lineup from that blueprint with deterministic per-slot local variation across the ocean substrate.
- **Swarm birth**: random swarms are now born as **two chromosomes totaling 4-18 agents**, with each chromosome constrained to **2-9 agents** and the realized chromosome sizes recorded in each chromosome blueprint.
- **Governance**: governance is now determined by **total swarm size parity**, per the intended model:
  - **odd total agent count -> alpha-led**
  - **even total agent count -> democratic**
- **Decision flow**: the decoder now consumes ordered genome segments against local ocean neighborhoods and gradients to produce action scores, bounded raise amounts, and bounded confidence.

## Phase 1 behavior preserved

The existing poker engine remains in place and still provides:
- dealer/button rotation
- 5 / 5 blinds
- preflop / flop / turn / river flow
- showdown evaluation
- chip conservation tracking

## What changed in this milestone

- upgraded `src/core/ocean.h/.cpp` with circular substrate helpers, diffusion, deposits, windowed reads, and gradients
- reworked `src/core/genome.h/.cpp` from one flat placeholder parameter list into ordered ocean-index bands plus structure genes
- updated `src/core/decoder.cpp` to consume the richer genome/ocean model instead of a thin sequential placeholder pass
- upgraded `src/core/chromosome.h/.cpp` so chromosomes carry a blueprint genome and spawn agent lineages from it
- upgraded `src/core/swarm.h/.cpp` so swarm creation and invariants reflect two chromosomes and a 4-18 total agent population
- updated `src/main.cpp` demo output to show total agents, chromosome split, and governance mode from the new birth model
- expanded `test/test_poker.cpp` to verify ocean substrate behavior, swarm birth/gene invariants, governance parity, and 200 random valid swarm decisions

## Invariants now enforced

The implementation now enforces these structural rules in code/tests:
- every swarm has **exactly two chromosomes**
- every swarm has **4-18 total agents**
- each chromosome has **2-9 agents**
- chromosome sizes sum exactly to total swarm size
- each chromosome blueprint records the same `chromosome_agent_count` as the realized chromosome size
- odd total swarm size => `alpha_led`
- even total swarm size => `democratic`
- genomes always contain ordered ocean-index bands for sensory/hidden/action/modulation access
- random swarms still always produce valid poker decisions with normalized action scores, bounded confidence, and legal raise ranges

## Design choice made for ambiguous areas

The project vision clearly calls for two chromosomes, structured swarm birth, and governance tied to total swarm size, but it does not yet fully specify the exact biological meaning of chromosome composition at this stage. For this milestone, the chosen interpretation is:
- a chromosome is a **lineage blueprint**
- the chromosome blueprint genome determines both the chromosome's internal composition and the number of agents instantiated from it
- individual agents are local variants of that blueprint rather than unrelated random genomes

This keeps the implementation honest and structurally aligned without pretending that full heredity, mating, mutation, or lifecycle systems already exist.

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp -o test_poker.exe`

### Test command

- `test_poker.exe`
- Verified scope:
  - poker evaluator ordering
  - poker tiebreak behavior
  - chip conservation
  - 5/5 blind logging
  - heads-up blind order
  - ocean circular substrate behavior, diffusion/deposit helpers, and local reads
  - 200 random swarms satisfying birth/governance invariants and producing valid decisions

### Runtime sanity run

- `swarm_poker_sim.exe --hands 10 --quiet`
- The executable prints one sample structured swarm decision and then the poker summary.

## Honest current limitations

Still intentionally deferred after this alignment pass:
- no real mutation/crossover, reproduction, or inherited selection loops yet
- no persistent population, lifecycle, birth/death economy, or inter-generation evolution yet
- no social memory, deception, coalition behavior, status contests, or long-horizon politics yet
- no direct live-table feature extraction from the poker engine into the swarm state vector yet
- the decoder is still a compact hand-built approximation, not a rich learned neural architecture
- no side-pot engine or true all-in continuation yet in the poker layer
- no analytics/dashboard layer yet

## Readiness

**Yes, this is now a truthful point to begin Phase 3**, because the current Phase 2 core is no longer just a skeletal placeholder. The project now has:
1. structured two-chromosome swarm birth
2. correct governance parity behavior based on total swarm size
3. chromosome blueprints that meaningfully determine internal composition
4. genomes that reference the ocean as ordered substrate bands rather than one flat placeholder list
5. tests proving the above invariants and decision validity

Recommended next step:
1. connect richer real poker table features into the swarm state vector
2. then add controlled mutation/crossover and population management on top of this structure
3. only after that move into social/economic lifecycle systems
