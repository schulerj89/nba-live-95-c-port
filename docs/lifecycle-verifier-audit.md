# Independent lifecycle verifier audit

2026-08-30, working changes above `2723af6`. Auditor inspected the retained
native traces, fixture, generated recomp, disassembly and production C before
running the candidate verifier. No implementer summary was used as proof.

## Current verdict after repair

**PASS for the bounded verifier repair.** All three independently reproduced
native-protocol defects below are now rejected, the five adversarial test
methods pass, and a fresh independent retained-evidence replay still matches
all twelve output words. Recheck outputs:
`.analysis/ownership-20260830/lifecycle-native-protocol-mutations-repaired.json`
and `lifecycle-auditor-independent-repaired.json` in the same directory.

The implementation now enforces exact native unsigned integer words,
nondecreasing frame order, well-formed PCs and the terminal event/PC pairing.
No expectation values or native captures changed. The original failure is
retained below so the audit does not erase the problem or its resolution.

## Initial verdict, superseded by the repair above

**FAIL for the native-capture validation protocol; PASS for the repaired
three-word C replay.** The protocol issues below must be resolved before
accepting this methods checkpoint. Neither verdict is whole-lifecycle parity.

## Evidence directly inspected

- `tools/match_lifecycle_expiry_probe.c`: `--project` parses only inputs and
  emits production period/clock/latch. Expected values cannot influence the
  result. `--self-test` explicitly labels manually decremented clocks and
  skipped presentation as C-only regression.
- `tools/verify_match_lifecycle.py`, `tools/test_match_lifecycle_verifier.py`,
  and `build.ps1`'s separate self-test/projection/adversarial invocations.
- `tests/fixtures/match-lifecycle-expiry-witnesses.json`: all four cases and
  twelve output words.
- `.analysis/match-lifecycle-native-20260829/{q1,halftime,regulation_tie,regulation_final}/events.jsonl`:
  215 + 216 + 211 + 209 = **851 rows**, not 851 independent lifecycle runs.
  Many rows repeat `$87:95E9` while the ROM waits within one frame.
- `.analysis/violation-oob-reference-20260829/bank87.c`:
  `L_9766_M0X0`, `L_976E_M0X0`, `L_9779_M0X0`, `L_979A_M0X0` read/increment
  `$0926`, compare scores at raw period four, and store five for a final.
- `.analysis/gameplay85-closure-ghidra/gameplay85_bank86_listing.txt`:
  `$86:DD2D-$DD44` chooses OT from `$86:E392` when period >=4, otherwise
  copies `$0A0C` to `$0928`.
- `.analysis/jump-wire/gates/shot_state_bank87.txt`: `$87:8EB2-$8ECF`
  latch/owner/unsigned-low-ball/signed-receiver gate.
- `src/nba_tipoff.c`: `match_finish_period`, `match_restart_period`, and
  `nba_tipoff_step_match_lifecycle` follow the represented normal-case
  period/score/clock branch. Other effects are outside this projection.

The direct replay report is
`.analysis/ownership-20260830/lifecycle-auditor-independent.json`.
It reproduces all four projected outcomes with no changed fixture values:

| Case | Native seed (period, score) | Native terminal PC | Period / clock / latch |
|---|---|---|---|
| Q1 | 0, 10–8 | `$86:DD47` | 1 / 43200 / 0 |
| Halftime | 1, 10–8 | `$86:DD47` | 2 / 43200 / 0 |
| Regulation tie | 3, 10–10 | `$86:DD47` | 4 / 18000 / 0 |
| Regulation final | 3, 10–8 | `$87:97A0` | 5 / 0 / 1 |

All four current adversarial test methods pass, including a mutation of each
of the twelve expected words. This fixes the old fixture-substring test that
did not consume native results.

## Blocking protocol findings

Independent in-memory mutations of native rows were all accepted by
`audit_native`; the original captures were not edited. Reproduction output:
`.analysis/ownership-20260830/lifecycle-native-protocol-mutations.json`.

1. Change the Q1 terminal PC to `000000` while retaining its event label and
   words: accepted. The verifier must require the correct event/PC pairing,
   not trust the event label alone. `$86:DD47` is the next-clock boundary;
   this retained final trace ends at `$87:97A0`, not the later return scene.
2. Change native Q1 terminal `$0926` from integer `1` to boolean `true`:
   accepted through Python equality. Require exact integer types and unsigned
   word range for compared native input/output words, just as the fixture does.
3. Change the terminal frame to `0` after frame `5234`: accepted. Require
   unsigned, nondecreasing frame order. Same-frame repeated native instructions
   are valid and should not be removed or mistaken for independent calls.

Add adversarial tests that mutate native rows, not only the compact fixture,
then rerun the retained-capture check. Auditor will append the resolved verdict.

## Scope and remaining caveats

- The three represented terminal words match. Frame timing, every intervening
  write, roster/stamina, PPU, audio, arbitrary flight-at-horn cases and full
  production caller state are **not** part of this native comparison.
- Native capture seeds clock=1 before `$85:EDC6`; C adapter starts at clock=0
  after that writer. The adapter intentionally skips presentation waits and
  calls lifecycle directly. That is a bounded state projection, not normal
  menu-to-match or same-entry full-state parity.
- Current capture script also writes Mode, team selection and Player Setup
  choices. The compact fixture says “natural Exhibition CPU-vs-CPU” but its
  controlled-write list lists only the later expiry seed. These setup
  interventions must remain disclosed. Current script is newer than these
  original event files (more fields and a later final capture endpoint).
  Original run script/executable/ROM hashes were not retained with this corpus.
  A fresh hash-recorded capture remains desirable.
- The raw trace does not include `$17B1`; its `$0A0C=43200` and tied OT result
  18000 support the documented setting, but are not a captured complete
  configuration block. The separate direct-ROM table test covers all eight
  clock values; this four-case projection uses setting 3 only.
- Final evidence here stops at postgame entry `$87:97A0`. It does not prove
  complete postgame, return, save, presentation, or final screen behavior.
- The inherited fixed presentation waits are based on one scripted input
  schedule. These tests deliberately do not grant them timing parity.
- `build.ps1` can run the compact fixture without ignored raw captures; that
  is a recorded projection regression. Use `--native-root` when claiming
  fresh validation against retained raw evidence.
