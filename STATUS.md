# Project status

Last updated 2026-08-25. Live numbers regenerate with
`python tools/progress.py --write docs/progress.md`; this file records the
methodology and frozen milestone snapshots.

## Where the port stands

The playable path covers the Nintendo license through Player Setup and into
live tip-off gameplay (see `README.md`). Verification no longer relies on
screenshots or recollection: every claim below is derived from machine
evidence that can be recomputed on demand.

## First-ever measurement — 2026-08-25 baseline

**30.4% of observed executed ROM code is documented by the port; 0.04% is
ground-truth verified.**

| metric | bytes | % of executed |
|---|---|---|
| executed (denominator) | 27,901 | 100.0% |
| documented by port provenance | 8,473 | 30.4% |
| verified against ground truth | 10 | 0.04% |

| bank | executed | documented | % | contents |
|---|---|---|---|---|
| $00 | 30 | 0 | 0.0% | vectors |
| $80 | 7,517 | 128 | 1.7% | OS/scheduler/runtime — biggest gap |
| $81 | 2,589 | 10 | 0.4% | |
| $82 | 1,937 | 418 | 21.6% | menus/Team Select |
| $83 | 1,043 | 0 | 0.0% | |
| $84 | 195 | 0 | 0.0% | |
| $85 | 5,095 | 3,361 | 66.0% | actor physics — strongest area |
| $86 | 6,114 | 3,528 | 57.7% | ball/AI/contact logic |
| $87 | 3,381 | 1,028 | 30.4% | rendering/dispatch |

Function-level: the NBA-Live-95-Recomp statically discovered 136 functions
(banks 00/80/81/82 only — its analysis stops at indirect dispatch). 126 of
them execute in our captures, but only 6 are referenced by port provenance.
The recomp therefore holds reference C implementations for nearly all of the
bank-$80 runtime the port has not yet documented.

Verified-routine ledger (`docs/verified-routines.json`): 1 routine —
`$80:CEE7` (`nba_gameplay_rng_next`), 500 live in-game calls replayed through
the compiled C with 0 mismatches, including the `$07F6`-zero recovery path.

## Current verified checkpoint — 2026-08-25

The ledger now contains 3 routines and 131 executed bytes (0.47% of the
27,901-byte observed denominator). The newest tip-off/gameplay routine is
`$85:F347-$F3BA`, `nba_gameplay_target_direction`: 3,000 live calls replayed
with 0 mismatches and 0 orphan exits. Both return paths and every direction
value 0-8 were observed; the probe compares the coupled `$AA` distance and
`$B2` direction outputs.

The next small post-tip checkpoint verifies `$86:BAA2-$BAFA`, the shared
player-grab prefix. Twenty-five live pass/rebound catches replay with zero
mismatches across 19 outputs, including both sides of the `$0080` movement
threshold. The port now preserves team ownership history and raw `$09A6`,
which were previously missing from its represented state. The ledger is now
4 routines and 163 executed bytes (0.58% of the observed denominator).

The first CPU-owner branch and its terminal dribble fallback are now verified
too. `$86:BAFD-$BB14` covers 24 ordinary mode-11 installs plus one live
mode-14 preservation; `$86:E593-$E5AA` covers 188 idle/catch pose-12 results
and 312 moving-owner pose-5 results. The larger `$86:E4A7-$E592`
proximity/facing selector remains explicitly unverified for the next small
increment rather than being hidden behind the verified fallback. The ledger
now contains 6 routines and 182 executed bytes (0.65% of the observed
denominator).

`$86:E4A7-$E4C4`, the opening mode-11 dribble gate, is now verified across
1,000 live calls: 21 vertical skips, 425 direct fallbacks, and 554
continuations into `$E4C7`, with zero mismatches. The downstream fallback and
catch-mode suites were replayed again after integration and remain exact. The
ledger now contains 7 routines and 195 executed bytes (0.70% of the observed
denominator).

The next selector slice, `$86:E4C7-$E4F3`, is verified across 1,000 live
calls: 841 fallbacks, 3 latched-owner paths, and 156 unlatched continuations.
The port now performs the ROM's paired-side, speed, and distance gates and
copies paired `+$86` direction into owner `+$50`. The unlatched pose decision
starting at `$86:E545` remains the next bounded increment. The ledger now
contains 8 routines and 213 executed bytes (0.76% of the observed
denominator).

The unlatched continuation `$86:E545-$E592` now copies requested facing into
actor `+$4E` and selects dribble base 9/11 from velocity relative to that
facing. All 158 live calls replay with zero mismatches (55 pose 9, 103 pose
11), and a full WRAM changed-byte audit confirms the slice writes only
`+$38/+$4E`. This corrected trajectory also exercises deferred shot starts;
the 63,800-frame CPU regression and refreshed frame anchors pass. The ledger
now contains 9 routines and 240 observed-executed bytes (0.86% of the
denominator); sparse execution inside this branch means only 27 new observed
bytes enter the metric.

## How each number is captured

All three inputs are machine-generated; none are maintained by hand.

1. **Executed code (denominator).** Mesen Lua captures register wide exec
   callbacks over banks `$80-$8F` while driving the game
   (`mesen_tipoff_capture.lua` and friends), then write every executed
   address as merged `XXXXXX-YYYYYY` ranges into `.analysis/**/exec_*.txt`.
   `tools/progress.py` takes the union of all of them. *Known blind spot:*
   only gameplay captures emit these files today, so the denominator
   under-counts menu/title code; a full-session coverage capture would
   complete it.
2. **Documented (numerator 1).** `tools/progress.py` parses every
   `$XX:XXXX` / `$XX:XXXX-$YYYY` provenance comment in `src/*.c` and
   intersects those ranges with the executed set. Keeping the
   provenance-comment convention is what keeps this metric honest.
3. **Verified (numerator 2).** A routine enters
   `docs/verified-routines.json` only after passing emulator ground-truth
   replay: `tools/mesen_func_vectors.lua` records real in-game calls
   (entry/exit CPU + WRAM snapshots, driven into live CPU-vs-CPU gameplay
   with `NBA95_VEC_DRIVE=1`), and `tools/verify_func_vectors.py` replays
   every vector through a small MSVC probe built against the actual port
   sources, diffing each output against the ROM's recorded exit state.
4. **Recomp function set.** Function definitions (`bank_XX_YYYY`) parsed
   from `../NBA-Live-95-Recomp/generated/bank*.c`.

Supporting tiers (details in `tools/README.md`):

- **Frame lockstep:** `mesen_tipoff_capture.lua` → `gameplay_rom.jsonl` vs
  the port's trace via `compare_gameplay_traces.py` — first divergent frame
  and field pinpoint the broken subsystem.
- **Golden regression:** `tools/trace_hash.py` freezes a lockstep-passing
  trace as per-frame hashes so later changes can't silently regress it.

## Next targets

- Add a full-session exec-coverage capture so the denominator includes
  title/menu code.
- Burn down bank `$80` using the recomp's generated C as reference,
  verifying each routine with the vector pipeline
  (`docs/progress.md` "largest undocumented executed regions" is the
  prioritized queue; `$80:81A8-$80:82A4` is the biggest).
- Next clean vector target: `$86:D549` pose contact
  (`nba_gameplay_ball_pose_contact_index`, exits `$86:D5D8`/`$86:D5DA`) —
  needs DP-relative snapshot support in `mesen_func_vectors.lua`.
