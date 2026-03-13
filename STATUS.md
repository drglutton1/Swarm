# Swarm Poker Ecosystem - Status

## Stage A / Phase 5

Status: **verified milestone still complete, with curator corrective pass applied before Phase 6**.

The repo still includes the first social-layer substrate from Phase 5, but the important change in this pass is that the poker/evolution substrate has been corrected where the curator flagged real spec and rules issues.

## Corrective pass completed

### PASS 1 — Poker correctness repair

Implemented:

- **Side-pot payout eligibility**
  - replaced the prior single-pot-only settlement path with layered pot resolution based on contribution slices
  - showdown payout now respects eligibility per pot layer
  - an all-in player can win only the pots they actually covered

- **More realistic betting-round flow**
  - removed the one-raise-per-street limitation
  - re-raises are now allowed
  - short stacks facing a bet now **call all-in / call short** instead of being force-folded by engine logic
  - betting rounds now track a minimum full raise size and reopen action after a full raise
  - short all-in raises that do not satisfy the minimum full raise do **not** incorrectly reopen action

- **Rake by street instead of flat full-hand rake**
  - flop: **5**
  - turn: **+1**
  - river: **+1**
  - no flop reached => no street rake taken
  - chop settlement still follows the project’s current even-split/remainder rule

### PASS 2 — Spec alignment fixes

Implemented:

- default **reproduction cooldown** corrected to **10000** hands
- default **crossover probability** corrected to **0.01**
- random swarm birth distribution corrected from a flat `uniform(4,18)` total to:
  - first chromosome size = `randint(2,9)`
  - second chromosome size = `randint(2,9)`
  - total swarm size = their sum

## Files changed in this corrective pass

- `src/poker/table.h`
  - betting-round flow reworked
  - added side-pot-aware showdown distribution
  - added explicit street-rake application
  - added targeted test helpers used by the local test suite
- `src/poker/pot_manager.h`
  - supports incremental rake and partial-pot awards
- `src/evolution/lifecycle.h`
  - reproduction cooldown default set to 10000
- `src/evolution/crossover.h`
  - crossover default set to 0.01
- `src/core/swarm.cpp`
  - swarm birth distribution changed to independent 2..9 chromosome draws
- `test/test_poker.cpp`
  - added regression coverage for curator-requested cases

## Verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp src/core/ocean.cpp src/core/genome.cpp src/core/decoder.cpp src/core/agent.cpp src/core/chromosome.cpp src/core/swarm.cpp src/economy/chip_manager.cpp src/economy/transfer.cpp src/economy/inheritance.cpp src/evolution/lifecycle.cpp src/evolution/reproduction.cpp src/evolution/mutation.cpp src/evolution/crossover.cpp src/social/face.cpp src/social/info_exchange.cpp src/social/mate_selection.cpp -o test_poker.exe`

### Tests / runs

- `test_poker.exe`
  - Result: **PASS**
- `swarm_poker_sim.exe --hands 10 --quiet`
  - Result: runs successfully
  - Sample result from local verification: `hands=10`, `rake=30`, `chips_ok=true`

### Regression coverage now includes

- uneven all-in **side-pot** resolution
- **multi-raise** / action reopening flow
- short-stack **all-in call** instead of forced fold
- **street-based rake** and chop remainder handling
- reproduction cooldown default = **10000**
- crossover default = **0.01**
- swarm birth distribution invariants under the new `2..9 + 2..9` rule

## Honest current limitations

This pass materially fixes the curator’s flagged correctness issues, but it is **not** a claim that the poker engine is now full production-grade no-limit Hold’em.

Still true:

- player action selection is still a **simple stochastic house policy**, not a true strategy engine
- there is no full dealer/button/blind/state machine abstraction separated from the table header yet
- rake is now applied by reached street, but it is still a **project-rule implementation**, not a casino-rule engine with caps, thresholds, or jurisdiction variants
- side-pot settlement is now materially correct for contribution eligibility, but the overall poker engine remains a compact simulation layer rather than a full tournament/cash-game rules implementation
- the newly added testing helpers in `table.h` are practical for regression coverage, but they are not yet a polished public API

## Readiness

**Yes — the project now looks ready for the next governance/RNG follow-up pass.**

Reason: the curator-blocking poker correctness and spec-default mismatches called out in this pass have been addressed and locally verified, so Phase 6 should no longer be built on top of the previously known side-pot / betting-flow / default-value errors.
