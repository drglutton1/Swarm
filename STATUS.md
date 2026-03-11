# Swarm Poker Ecosystem - Status

## Stage A / Phase 1

Status: **verified milestone complete** for the current poker-engine foundation pass. The simulation now runs a materially more correct no-limit Texas Hold'em hand flow than the original ante-only baseline, builds locally with MinGW g++, passes the local test suite, and preserves chips under both test and benchmark runs.

## Phase 1 direction folded into this pass

This pass keeps the scope at the poker-core layer and aligns it with the current Phase 1 direction:
- real Hold'em deal flow instead of immediate random showdown
- dealer/button rotation
- blind posting
- preflop / flop / turn / river progression
- per-street action loop with fold / check / call / single-raise behavior
- showdown using best-of-seven evaluation
- chip/rake accounting that remains conserved and testable

## Authoritative blind decision

Per user decision, blinds are now locked to:
- **small blind = 5**
- **big blind = 5**

This is reflected in:
- runtime construction in `src/main.cpp`
- tests in `test/test_poker.cpp`
- config notes in `config/default.toml`

## What changed in this milestone

- kept and verified the in-progress refactor from ante-only hand resolution toward street-by-street Hold'em flow in `src/poker/table.h`
- fixed a real correctness bug in heads-up play: the button/dealer now posts the small blind and the other funded player posts the big blind
- kept preflop action starting after the big blind, with later streets starting after the dealer/button
- preserved showdown and uncontested-pot payout handling through the pot manager
- tightened tests so the suite now checks:
  - evaluator ordering
  - evaluator tiebreak behavior
  - long-run chip conservation
  - verbose logging for Hold'em streets
  - **5/5 blind logging specifically**
  - **correct heads-up blind order**

## Local verification performed

### Build commands

- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc src/main.cpp -o swarm_poker_sim.exe`
- `C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -Isrc test/test_poker.cpp -o test_poker.exe`

### Test command

- `test_poker.exe`
- Result: `PASS: test_poker`

### Simulation sanity run

- Command: `swarm_poker_sim.exe --hands 100 --quiet`
- Result:
  - `hands=100`
  - `initial_chips=8000`
  - `final_chips=7900`
  - `rake=100`
  - `chips_ok=true`
  - elapsed: `1.8092 ms`

Interpretation: 100 chips were removed as rake across 100 hands, and `final_chips + rake == initial_chips`, so conservation held.

### Benchmark run

- Command: `swarm_poker_sim.exe --benchmark 100000`
- Result:
  - `hands=100000`
  - `initial_chips=8000000`
  - `final_chips=8000000`
  - `rake=0`
  - `chips_ok=true`
  - elapsed: `1737.75 ms`

Interpretation: with rake disabled, total chips remained exactly constant over 100k hands.

## Honest current limitations

Still intentionally simplified for this milestone:
- no side-pot engine yet
- no true all-in continuation logic; short stacks facing a bet currently fold rather than create capped all-in states
- betting policy is still intentionally simple/random, not strategic
- raise sizing is still minimal/simple
- no tournament blind schedule
- no agent memory, personality, deception, coalition, or evolutionary systems yet
- no structured analytics output or dashboard yet

## Readiness

This repo is now in a truthful state to proceed to the next Stage A step.

Recommended next focus:
1. proper all-in handling
2. side pots
3. more realistic betting policy / action abstraction
4. structured hand-history and aggregate metrics output
