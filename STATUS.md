# Swarm Poker Ecosystem - Status

## Stage A / Phase 7 UBI balance pass

Status: **implemented and validated, but the requested smoke test still collapses behaviorally. Accounting stays honest; UBI slows cash starvation a bit, yet it does not by itself stabilize the ecosystem at the requested test size.**

## What changed in this pass

### 1) UBI was added to the simulation config and block loop
`SimulationConfig` now includes:
- `ubi_amount = 150`
- `ubi_interval_blocks = 30`
- `ubi_enabled = true`

Integration point:
- UBI is applied in `Simulation::step_block()`
- ordering is now:
  1. process deaths
  2. apply UBI
  3. attempt social reproduction
  4. schedule activity / tables / costs
- only living swarms with bankroll `> 0` receive UBI
- all injections go through `ChipManager`, so cumulative injection accounting remains honest

### 2) Lifecycle timing was accelerated
Lifecycle defaults are now:
- `maturity_start_hands = 3000`
- `reproduction_cooldown_hands = 3000`
- `max_offspring = 12`
- `old_age_start_hands = 90000` (unchanged)
- `death_hands = 100000` (unchanged)

### 3) Activity costs were lowered
Per-block activity burns are now:
- `active_rest = 3`
- `sleep = 1`

These still route through `ChipManager`.

### 4) Stronger telemetry was added
Per block, stats now expose the requested/ecosystem-relevant fields including:
- `alive_count`
- `deaths_this_block_age`
- `deaths_this_block_bankruptcy`
- `births_this_block_reproduction`
- `births_this_block_injection`
- `total_chips_in_play`
- `total_chips_injected`
- `total_chips_burned`
- `tables_active`
- `hands_resolved`
- `avg_bankroll_alive`
- `median_bankroll_alive`
- `max_bankroll`
- `parity_ratio`
- per-block gene means / lineage depth snapshots

Per 100 blocks, JSON now also includes:
- `action_distribution`
- `avg_risk_gene_alive`
- `avg_honesty_gene_alive`
- `avg_skepticism_gene_alive`
- `reproduction_success_rate`
- `avg_lineage_depth`
- `max_lineage_depth`
- `top_dynasties_living_descendants`

### 5) Lineage telemetry support was added
Population lineage metadata now tracks founder/root IDs and depth so the 100-block summaries can report lineage depth and leading living dynasties.

### 6) CLI support for validation improved
`swarm_poker_sim --simulate-blocks ...` now accepts `--ocean-size N`, which was needed for the requested smoke test configuration.

## Donation unlock
I did **not** find a donation unlock feature in the current code/config. No fake support was added.

## Validation performed

### Build + tests
- Rebuilt `test_poker.exe`
- Rebuilt `swarm_poker_sim.exe`
- `test_poker.exe` passes

### Phase 1 smoke test
Command:
- `build\swarm_poker_sim.exe --simulate-blocks 500 --population 200 --ocean-size 10000 --seed 1337 --quiet`

Saved output:
- `validation_smoke_200_500_ocean10000_seed1337.json`

Observed at block 500:
- `chip_accounting_ok = true`
- `alive_count = 1`
- births in last 100 blocks = `0`
- blocks with any tables in last 100 blocks = `0`
- final `tables_active = 0`
- final `hands_resolved = 0`
- final `total_chips_in_play = 1071910`
- final `total_chips_injected = 1072400`
- final `total_chips_burned = 490`

## Honest assessment

### What improved
- accounting remains correct under UBI injections + activity burns + table settlement
- observability is materially better
- lifecycle pressure is less delayed
- rest/sleep are less punitive

### What did **not** improve enough
- the requested smoke test still collapses badly
- `alive_count > 50 at block 500` is **false**
- `births > 0 in last 100 blocks` is **false**
- table formation is **not stable** in the smoke test
- UBI accumulates onto a tiny survivor tail instead of producing healthy broad ecological continuity

So the honest result is:
- **engineering correctness:** better
- **telemetry / debugability:** much better
- **ecosystem balance:** improved only marginally, not enough to call stable
- **scientific/emergent-readiness:** still no

## Files touched in this pass
- `src/evolution/lifecycle.h`
- `src/main.cpp`
- `src/sim/population.h`
- `src/sim/population.cpp`
- `src/sim/simulation.h`
- `src/sim/simulation.cpp`
- `src/sim/statistics.h`
- `src/sim/statistics.cpp`
- `test/test_poker.cpp`
- `STATUS.md`
- `darwin_core.yaml`
