# Swarm Poker Ecosystem - Status

## Stage A / Phase 7 stabilization pass

Status: **stabilization pass implemented; accounting remains honest; telemetry is materially better; scheduler is still behaviorally weak and not yet ready for a confident serious ecosystem run.**

This pass added the missing activity-economy wiring, expanded integrated observability, hardened reproduction/decision plumbing, and revalidated the integrated loop.

## What changed in this pass

### 1) Activity costs are now real
Implemented per-block activity burns aligned to the requested 30-hand-equivalent costs:
- `active_rest`: **15 chips**
- `sleep`: **5 chips**

Integration details:
- activity costs are burned through `ChipManager`
- accounting stays invariant-checked after activity burns, poker transfers, and rake burns
- per-block stats now expose `active_rest_cost` and `sleep_cost`

### 2) Scheduler cadence got a stabilization rebalance
The activity scheduler now:
- still enforces the **minimum 30% sleep ratio** (covered by test)
- adds explicit rest-pressure alongside sleep-pressure instead of defaulting almost entirely to play/sleep extremes
- uses modest early-block play encouragement plus anti-repeat penalties to reduce degenerate same-mode streaking
- keeps old-age/reproduction state influencing cadence, but less crudely than before

Net result from validation:
- table formation improved a bit versus the prior corrective-pass baseline
- but the ecosystem still collapses behaviorally after a short active window
- the run is still dominated by bankruptcy rather than healthy long-lived table ecology

So: **better, but not fixed enough**.

### 3) Statistics / observability are much better
Integrated stats output now includes:
- decision/action counts:
  - `fold_actions`
  - `check_call_actions`
  - `raise_actions`
- coarse decision diversity metric:
  - `decision_diversity` (normalized Shannon entropy over the three action buckets)
- reproduction telemetry:
  - `reproduction_attempts`
  - `reproduction_successes`
  - `offspring_born`
- death causes:
  - `deaths_age`
  - `deaths_bankruptcy`
  - `deaths_other`
- activity distribution by block:
  - `playing_swarms`
  - `resting_swarms`
  - `sleeping_swarms`
  - `block_play_ratio`
  - `block_rest_ratio`
  - `block_sleep_ratio`
- cumulative activity counters:
  - `cumulative_play_ticks`
  - `cumulative_rest_ticks`
  - `cumulative_sleep_ticks`

### 4) Robustness hardening
Also fixed / hardened:
- integrated raise mapping now safely degrades to check/call if a legal raise target does not exist
- reproduction now defensively refuses empty-genome offspring instead of crashing the run if a malformed blueprint ever appears
- death-cause accounting is now reported honestly; current validation deaths are overwhelmingly bankruptcy-driven

## Files changed in this pass
- `src/evolution/reproduction.cpp`
- `src/scheduler/activity_scheduler.cpp`
- `src/sim/population.h`
- `src/sim/simulation.h`
- `src/sim/simulation.cpp`
- `src/sim/statistics.h`
- `src/sim/statistics.cpp`
- `test/test_poker.cpp`
- `STATUS.md`
- `darwin_core.yaml`

## Verification performed

### Build
Verified with the existing direct MinGW commands:
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc ... -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc ... -o test_poker.exe`

### Tests
- `test_poker.exe`
  - Result: **PASS**

## Integrated validation

Validation command set:
- `swarm_poker_sim.exe --simulate-blocks 200 --population 100 --seed 1337`
- `swarm_poker_sim.exe --simulate-blocks 1000 --population 100 --seed 1337`

### 200-block run
Observed:
- chip accounting:
  - **healthy for all blocks**
- final population state:
  - **1 alive / 105 dead / 106 total historically tracked**
- offspring:
  - **6 births**
  - **6 reproduction attempts / 6 successes**
- table formation:
  - **8 blocks** formed tables
  - **18 total tables**
  - max `tables_formed` in one block: **8**
  - `hands_resolved_total`: **458** seated-hand participations
- activity distribution averages:
  - play: **0.9674**
  - rest: **0.0103**
  - sleep: **0.0223**
- costs incurred:
  - active rest: **390**
  - sleep: **575**
- death causes:
  - age: **0**
  - bankruptcy: **105**
  - other: **0**
- decision diversity:
  - average `decision_diversity`: **0.0209**

### 1000-block run
Observed:
- chip accounting:
  - **healthy for all blocks**
- final population state:
  - **1 alive / 105 dead / 106 total historically tracked**
- offspring:
  - **6 births**
  - **6 reproduction attempts / 6 successes**
- table formation:
  - still only **8 blocks** formed tables total
  - **18 total tables**
  - max `tables_formed` in one block: **8**
  - `hands_resolved_total`: **458**
- activity distribution averages:
  - play: **0.9935**
  - rest: **0.0021**
  - sleep: **0.0045**
- costs incurred:
  - active rest: **390**
  - sleep: **575**
- death causes:
  - age: **0**
  - bankruptcy: **105**
  - other: **0**
- decision diversity:
  - average `decision_diversity`: **0.0042**

## Honest assessment

### What is better now
- activity costs are implemented and accounted for correctly
- integrated output is much more useful for debugging dynamics
- table formation is somewhat less anemic than the earlier corrective-pass baseline
- integrated accounting still looks trustworthy even with activity burns + rake plumbing
- obvious crash edges around raise targeting / malformed offspring are hardened

### What is still not good enough
- the scheduler is **still not producing a healthy sustained ecosystem cadence**
- the observed population dynamics are **not healthy**:
  - almost all deaths are bankruptcy
  - activity remains overwhelmingly concentrated into short-lived play bursts
  - rest usage is still very low
  - table formation still dries up quickly
- the current block-level play/rest/sleep ratios are informative, but because the population collapses fast they do **not** describe a mature steady-state ecology
- this pass does **not** justify any claim of robust emergence or stable evolutionary pressure

## Readiness assessment

**No — after this stabilization pass, the project is still not ready for a serious simulation run in the scientific / emergence-analysis sense.**

More precisely:
- **engineering correctness:** improved and usable
- **accounting correctness:** good
- **telemetry / observability:** much better
- **behavioral ecology / scheduler health:** still weak

It may be acceptable to do **guarded exploratory runs** for further tuning, but it is **not yet ready** for a cautious serious run if the goal is meaningful ecosystem behavior rather than plumbing verification.
