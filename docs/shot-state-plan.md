# Natural shot selection and shot-state inputs

Started 2026-08-27 from `272512f`. Census recorded before runtime code changes.
Implementation, independent replay and full regression suite complete.

## Pre-code instruction census

Counts are unique decoded Ghidra instruction starts, not bytes, generated C
lines, execution frequency, or overall game completion. An incomplete slice
can contain existing approximations; these are instructions pending complete
translation/verification, not necessarily 239 entirely absent operations.

| Area | ROM range | Decoded | Pending baseline |
|---|---|---:|---:|
| Natural selector | 86:B625-B6D2 | 68 | 0; natural-flow verification remains |
| Made-shot modifier update/reset | 85:A081-A0B7 | 21 | 21 |
| Late-game assistance selection | 85:A0B8-A0EA | 20 | 20 |
| Period assistance reset | 86:DD80-DD88 | 3 | 3 |
| Active-player fatigue update | 87:98EA-9969 | 57 | 57 |
| All-24-player stamina recovery | 87:996A-99C2 | 44 | 44 |
| Timeout/period stamina grant | 87:985D-987D | 15 | 15 |
| Fixed stamina grant | 86:8468-8495 | 20 | 20 |
| All-24-player initialization | 86:DA49-DA60 | 11 | 11 |
| Fatigue/clock cadence | 85:EDC6-EE3D | 45 | 45 |
| Fatigue timer initialization | 87:8DF3-8DF8 | 2 | 2 |
| Scheduler call | 87:8EF3-8EF6 | 1 | 1 |
| **Total pending** | | | **239** |

Reproduce: `python tools/count_shot_state_instructions.py --listing-dir
.analysis/shot-state-ghidra-20260827`. Listings come from
`tools/ghidra/DumpShotStateMap.java`, with 16-bit M/X context. Inline tables
are excluded. The census script rejects missing entries, truncated
instructions, and suspicious BRK/COP decoding.

## Source findings and scope

- Previous `hot_team` naming was misleading. `$09C0` is a late-clock
  trailing-team assistance selector, gated by CPU Assistance `$17C1`;
  it is not a generic hot-streak system. Compare the **pre-basket** score.
- Actor `+$B2` increments for the shooter and clears on the opposing five
  actors when a basket is made. The sibling `+$B4` counters also clear.
- Fatigue is statistics-record `+$18`, not actor `+$18`. Track all 24
  roster records; active actors reference their roster's record. Active
  drain is gated by `$17E7`, while the all-roster recovery call is not.
- Fatigue cadence uses `$09C2` and actual clock-running branches, not simply
  every rendered frame. Initialization starts this counter at 1000.
- Stamina initialization/grant helpers are in scope; implementing entire
  timeout menus, substitutions and period-presentation systems is not.
  Separate verified helper bodies from any caller integration still pending.

## Phases and acceptance

1. Map/count first (complete). Preserve this baseline while adding discovered
   scope explicitly, never silently altering the denominator.
2. Observe natural selector inputs/outcomes, compare them with C, and fix
   evidenced missing branches. Never force a frequency or inject qualifying
   inputs and call that natural selection.
3. Port modifier, assistance, fatigue and timing/grant helpers from focused
   recomp/Ghidra. Capture natural and clearly labeled controlled-ROM witnesses;
   store durable replay tests. Keep roster/timing tables in the asset pack.
4. Integrate CPU runtime, expose relevant debug state, run full regression
   and inspect changed frames. Document exact verified/pending boundaries,
   commit/push to main, and report pending work in a table.

## Result and remaining boundaries

| Area | Pending baseline | Verified this checkpoint | Remaining in listed slices |
|---|---:|---:|---:|
| Natural selector body | 0 | 47 additional natural ROM calls; unforced C mode 17 | 0 |
| Made-run modifier | 21 | 21 | 0 |
| Trailing-team assistance and reset | 23 | 23 | 0 |
| Active fatigue / all-roster recovery | 101 | 101 | 0 |
| Stamina and timer initialization | 13 | 13 | 0 |
| Clock cadence / pre-actor call binding | 46 | 46 | 0 |
| Timeout/period grant helper bodies | 35 | 35 | 0 |
| **Total** | **239** | **239** | **0** |

This is **not** a claim that all surrounding gameplay is finished. The
35 grant-helper instructions are verified, but timeout/period caller flows
remain unimplemented. Pending caller work is not yet instruction-counted:

| Pending integration | Known anchor | Boundary |
|---|---|---|
| Timeout menu -> fixed stamina grant | 86:844E -> 8468 | Actual ROM timeout-menu capture proves helper, not a C timeout UI |
| Period/timeout recovery grants | 87:96FB / 9716 / 974B -> 985D | Helper replayed; full period/timeout orchestration deferred |
| Next-period reset and quarter-clock initialization | 86:DD80; current C initial clock 43200 | Existing tip frame-220 handoff/initial clock seed retained; no full-period claim |
| Substitutions/bench promotion | 87:98EA active mapping 3435 | All 24 records persist; changing the active lineup is a later feature |
| Wider natural selector distribution | 86:B625 and callers | One unforced C special proves reachability, not ROM frequency parity |

The clock helper replaces the old unconditional match-clock decrement,
30-Hz ownerless-only `$0930` decrement, and duplicate possession `$092C`
decrement. `$0A04` is now a distinct run-clock latch, seeded on a make;
it is not an alias for the inbound countdown `$092E`.

## Verification and provenance

- `tests/fixtures/shot-state-witnesses.json`: **342** zero-mismatch writer
  calls, 176 natural and 166 controlled. By kind: 70 make, 74 fatigue,
  16 recovery, 168 clock, 4 stamina init, 3 timer init, 4 reset, 2 variable
  grant, 1 fixed grant. All 39 controlled make cases are present, covering
  shooter sides, clock boundaries and asymmetric score differences.
- `tests/fixtures/natural-shot-selection-witnesses.json`: **47** additional
  natural ROM selector calls, all ordinary. Rejection reasons: moving 32,
  range 11, lane 1, facing 1, appearance 2. This run contains no eligible
  special and is not presented as a special-shot witness.
- Unforced C run: **145** selector calls (131 blocked lane, 13 moving,
  **1 special**), **27** made-run updates. Actor 4 selects pose `$15` at
  frame **50338**, jumps while retaining the ball, and releases at **50366**
  on upper phase 3. The selector was not loosened or frequency-forced.
- Runtime binding probe: 16,000 frames each with fatigue OFF and ON;
  210/106 updates and 7/7 made baskets respectively. The ON test explicitly
  seeds a controlled late-clock deficit and observes four assistance-enabled
  makes; neither test forces a basket. Checks real roster,
  boost, clock, config, bench preservation, actor stamina mirrors and
  pre-score counter/assistance commit order.
- Asset tests compare all **88 fatigue-table bytes**, all **348 roster
  stamina ratings**, and the existing 528 shot-table bytes to ROM. F12
  image comparison proves only the count header changed with the new entry.
- The long CPU regression passes: 63,800 frames, final **25–30**,
  2,184 exact-pass frames and 103 automatic action unlocks. The existing
  2,400-frame inbound guard and screenshot goldens are unchanged.
- Final full-suite: **PASS**, `build.ps1 -Test`, including intro/legal/title,
  setup/menu audio and transitions, team/player screens, Player Lab, gameplay
  assets, replay suites and 63,800 CPU frames. Log:
  `.analysis/shot-state-proof-20260827/full-regression.log`. The strengthened
  late-clock runtime binding probe also passed separately after that run.

The fatigue routine's timer store can be interrupted by `$85:EE27 INC
$09C2`. Capture retains every timer byte and writer PC. The verifier
reconstructs both the owned `$87:9900 STZ` and the independent interrupt,
checks the final ROM value, and compares C to the routine-owned result.
No timing tolerance or unexplained mismatch is discarded.

New natural paths exposed old regression assumptions, not new production
exceptions: mode 17 uses activities 1/3/FFFF and pre-integration attachment;
`$86:C0D7-C0EA/C127-C13A` can reset global wind-up activity when another
player is knocked down; a low launch can hit the rim in the same frame.
Tests now distinguish these exact paths while retaining ordinary assertions.

Fresh Ghidra labels/comments: `tools/ghidra/DumpShotStateMap.java`.
Listings: `.analysis/shot-state-ghidra-20260827/`. Focused recomp:
`.analysis/shot-state-recomp-20260827/generated/` (fatigue/recovery native
translations; bounded inline prefixes may use LLE dispatch, so those
wrappers alone are **not** treated as proof). Independent ROM entry/exit
captures and Ghidra instructions supply their oracle.

Completed raw captures: `.analysis/shot-state-writers-natural-20260827/`,
`shot-state-writers-final-20260827/`, `shot-state-timeout-final-20260827/`,
and `shot-state-make-tail-20260827/`. The last three names are also beneath
`.analysis/`. Earlier controlled/provenance diagnostic runs hit the default
Mesen timeout and are **not** included in the fixture.

Visual evidence: `.analysis/shot-state-proof-20260827/natural-special.mp4`
and `natural-special/frame_50350.bmp` / `frame_50366.bmp`. These are port
output, not artwork inputs. Gameplay uses the asset pack; neither Mesen
nor recomp is a runtime dependency.

Captured-address coverage: **6785 -> 6906 / 27901 (24.32% -> 24.75%)**,
118 ledger slices. This adds 121 captured address positions; it is not
239 new coverage positions, since some address ranges were already covered
by earlier broader slices. Keep the instruction census distinct.
