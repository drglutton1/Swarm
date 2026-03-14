# Swarm Poker Ecosystem - Status

## Stage A / Phase 7 corrective pass

Status: **critical Phase 7 blockers addressed; build/tests pass; integrated accounting now stays honest under rake.**

This pass fixed the issues that made the first Phase 7 substrate unsafe for serious simulation:
- integrated tables now use `swarm.decide()` instead of the poker engine's built-in random bot policy
- big blind defaults are back to the intended `5`
- chip accounting now supports externally burned rake and remains invariant-safe in the integrated loop
- population lookup is O(1) by swarm id for active swarms
- dead swarms are pruned from the active container and retained separately for bookkeeping
- default seeded population generation now uses the intended `randint(2,9) + randint(2,9)` chromosome-size distribution
- reproduction partner search no longer scans every swarm pair; it uses parity partitioning plus bounded candidate sampling

## What is fixed

### 1) Blind defaults corrected
- `SimulationConfig.big_blind` is now `5` instead of `10`
- the sample state in `main.cpp` was aligned with the same 5/5 blind baseline

### 2) Integrated chip accounting repaired
- `ChipManager` now supports `burn_external()` for rake that is already removed inside the poker table engine
- integrated table settlement now enforces the invariant:
  - **sum of ending swarm stacks = sum of starting swarm stacks - rake**
  - all non-rake poker transfers remain zero-sum between swarms
- simulation throws if a table block violates that invariant instead of silently drifting
- nonzero-rake integrated runs now keep `chip_accounting_ok=true`

### 3) Population lookup and dead-swarm cleanup
- active swarms now maintain an `id -> index` map for O(1) lookup
- dead swarms are moved out of the active swarm vector after death processing
- dead swarms remain available in `dead_swarms()` so bookkeeping and post-mortem lookup stay coherent

### 4) Swarm decisions are actually connected to poker play
- integrated table blocks no longer use the table's random action chooser
- the simulation now builds a real adapter:
  - `Table::DecisionContext`
  - `DecisionContext -> PokerStateVector`
  - `swarm.decide(ocean, state)`
  - decision mapped back into legal poker action (`fold`, `check/call`, `raise_to`)
- table execution is therefore now driven by swarm brains during integrated simulation

### 5) Population/reproduction scaling pass
- default seeded populations now follow the intended 2..9 + 2..9 distribution instead of the earlier narrow deterministic pattern
- partner search is now parity-partitioned with bounded candidate sampling instead of effectively all-to-all scans every block

## Tests added/updated

`test/test_poker.cpp` now additionally covers:
- external chip burns through `ChipManager`
- dead-swarm pruning into a graveyard container
- integrated simulation with nonzero rake while preserving accounting
- existing simulation tests still pass after the corrective pass

## Verification performed

### Build

Verified with the existing direct MinGW commands:
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc ... -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc ... -o test_poker.exe`

### Test result

- `test_poker.exe`
  - Result: **PASS**

### Integrated validation

Command run:
- `swarm_poker_sim.exe --simulate-blocks 1000 --population 100 --seed 1337`

Observed results:
- chip accounting status:
  - **healthy for all 1000 blocks**
  - non-rake chip movement stayed zero-sum within tables
  - only rake was counted as burned
- alive swarm count:
  - started effectively at **96 alive** (4 seeded into old-age-death threshold during first pass)
  - ended at **66 alive / 37 dead / 103 total historically tracked**
- offspring:
  - **3 births observed** over the run
- tables / activity:
  - max `tables_formed` in a block: **4**
  - only **5 blocks** formed any tables at all in this configuration
  - max `hands_resolved` in a block: **64 seated-hand participations**
- decisions varying:
  - integrated poker is now driven by `swarm.decide()` rather than random bots
  - this pass did **not** add deep action-distribution telemetry yet, so variability is not richly quantified in the JSON output
  - observed bankroll divergence and nonuniform table outcomes confirm the path is live, but decision diversity instrumentation is still a next-step gap
- rough performance:
  - local 1000-block / 100-swarm validation run completed in well under a second on this machine

## Honest current limitations / remaining risks

The blockers are fixed, but the ecosystem is **not** yet behaviorally mature:
- the scheduler currently becomes extremely sleep-heavy after a brief active window; most later blocks form no tables
- because of that, evolutionary pressure is still too sparse for meaningful emergence claims
- activity-cost pressure for `active_rest` / `sleep` was **not** implemented in this pass because the precise spec economics were not encoded clearly enough in the current codebase
- alpha-led governance was inspected and existing advisor-sensitive tests still pass, but this pass did not add integrated governance telemetry
- integrated output still lacks richer action-distribution / policy-diversity instrumentation

## Readiness assessment

**Yes — the project now looks ready for a cautious first real simulation run, but only as a guarded engineering run, not as a serious emergence study.**

What that means:
- the main correctness blockers are no longer lying to you
- accounting is trustworthy enough to run with rake
- integrated poker uses actual swarm decisions
- dead-swarm bookkeeping is sane

What it does **not** mean:
- not ready to claim robust social emergence
- not ready to claim reproduction dynamics are healthy
- not ready to claim the scheduler/activity economy is tuned
- not ready to skip the next pass of telemetry + scheduler pressure tuning
