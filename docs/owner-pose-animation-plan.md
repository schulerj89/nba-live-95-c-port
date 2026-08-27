# Latched-owner pose and special animation cadence

Started 2026-08-27 from `da1281b`. Baseline recorded before code changes.

| Requested slice | ROM range | Pending decoded instructions |
|---|---|---:|
| Upper state 13 (`$0D`) | 87:AE89-AEBC | 20 |
| Upper state 18 (`$12`) and dispatch | 87:ADBE-AE88 | 78 |
| Latched owner/CPU pose | 86:E4F5-E544 | 31 |
| Total | | 129 |

Counts are disassembled instruction starts, not address coverage, C lines,
or a percentage of the entire game. Existing approximations still require
complete ROM verification. Baseline listings: `.analysis/pending-census-20260827/`.

## Plan and acceptance

1. Read focused recomp and Ghidra listings, preserving 16-bit arithmetic and
   resource resolution order. Label/comment the corresponding subroutines.
2. Implement the two special cadence branches and actor `+$B0` state; adopt
   the latched pose selector in CPU gameplay. Use existing asset-pack tables.
3. Capture independent natural and labeled controlled-ROM entry/exit calls.
   Replay channel words, RNG, resource IDs, and pose/facing outputs exactly.
   Add durable regression fixtures and live binding checks, then run the full
   suite and inspect native port frames/video. Do not hide changed behavior
   by weakening existing gameplay assertions.
4. Update the measured ledger/status, rebuild and refresh the desktop
   shortcut, commit and push to main. Timeout/period callers, substitutions,
   and human controls are outside this increment.

## Findings

- The latched branch was incorrectly routed to the generic fallback in C.
  CPU actors use signed `+$8A - $11` to choose state 13 or 18 and facing.
- State 13 shares its randomly chosen phase with the lower body. The ROM
  resolves the lower resource *before* that upper-body phase write, so the
  new lower phase affects the next call, not this call's resource.
- State 18 retains a phase target/direction in actor `+$B0`; it advances at
  most one phase per call, wraps within eight phases, and only chooses a new
  target/timer once the current phase reaches the target. Preserve RNG order.

## Result

| Slice | Baseline pending | Captured/replayed instructions | Pending |
|---|---:|---:|---:|
| Upper state 13 | 20 | 20 | 0 |
| Upper state 18 + dispatch | 78 | 78 | 0 |
| Latched owner pose | 31 | 31 | 0 |
| Total | 129 | 129 | 0 |

Measured captured-address coverage changes **6,906 -> 6,917 / 27,901**,
**24.75% -> 24.79%**. Only 11 new address positions intersect the existing
execution-capture denominator; do not equate the 129-instruction branch census
with this older sampled address metric or inflate the denominator silently.

## Independent evidence and reproduction

`tools/ghidra/DumpOwnerPoseAnimation.java` disassembles the complete helper at
`$87:AD5B-$AEC2` and selector `$86:E4F5-$E544`, and saves named labels/plate
comments in the bank86/bank87 Ghidra projects. Final listings:
`.analysis/owner-pose-ghidra-20260827/owner_pose_bank86.txt` and `...bank87.txt`.
Focused recomp roots (`entry_mx:0,0`) are `ad5b end:aec3` and
`e4f5 end:e545`; generated exact M0X0 bodies are under
`.analysis/owner-pose-recomp-20260827/generated/`. CPU emulation/stack glue
is reference material, not runtime code or independent test output.

Run independent captures:

```powershell
.\tools\capture_owner_pose.ps1 -OutputDir .analysis/owner-pose-paths-20260827 -Controlled
.\tools\capture_owner_pose.ps1 -OutputDir .analysis/owner-pose-natural-long-20260827 -Frames 20000
```

Both final captures completed with zero orphan exits. The controlled pass
changes WRAM only at real native entries; the caller's dead-ball gate at
`$86:E4ED` ensures the rare latched fork executes. No PC, flags, stack, ROM,
or image injection. All controlled calls are labeled and their fields restored.
An early diagnostic used an invalid lower phase 1 for state 18's one-frame
lower descriptor; those rejected inputs were corrected to phase 0 and the
capture repeated. They are not silently counted as passing witnesses.

12,049 full calls match with zero channel/RNG/resource/pose mismatches.
The durable fixture retains 326 calls: 130 controlled (18 state13, 70 state18,
42 selector) plus 196 naturally entered calls, including seven latched-owner
calls. **The two special cadence states use controlled-ROM witnesses:** the
20k-frame natural ROM capture did not observe their cadence entries. Executed
PC lists prove all 129 requested instruction starts, not just branch labels.

```powershell
.\tools\build_vector_probe.ps1 -Name owner_pose_animation_vector_probe
python tools/verify_owner_pose_animation_vectors.py --normalized --vectors tests/fixtures/owner-pose-animation-witnesses.json --probe build/owner_pose_animation_vector_probe.exe --pack build/nba95_assets.pak --listing-dir .analysis/owner-pose-ghidra-20260827
```

Production initialization checks the latched selector -> locomotion -> cadence
binding for both distance boundaries and 40 ticks each, preserving `+$B0`,
RNG, desired/display facing separation and resource IDs. This is explicitly a
C binding test, not additional ROM coverage. Its held-pose fixture sets the
actual decision timer `+$60`, not the unrelated behavior timer `+$64`.
`owner_pose_runtime_probe` also requires valid asset-pack resources for both
states in two unforced 200k-frame games. State13/state18 first occur at
33,660/2,020 in Chicago-Orlando and 860/504 in the reverse matchup.

The old 63,800-frame natural-special existence check moved to that two-match
probe: exact animation RNG consumption removes the old short-window special.
Existence, airborne startup, phase >=3, release within 120 frames, ball owner,
ball state, shooter identity, animation and two-point value remain mandatory.
Orlando-Chicago now selects at 164,418 and releases at 164,446 (actor5, pose15).
The detailed 63,800-frame trace still checks every selector/made-run update
and every special it sees. Existing screenshot hashes and the 2,400-frame
dead-ball guard are unchanged. No shot was forced to satisfy the regression.

## Visual proof and remaining scope

Native port, default teams/seed, existing asset pack (no captured art):

- [State18 screenshot](../.analysis/owner-pose-proof-20260827/held-pose-18.png)
  and [181-frame video](../.analysis/owner-pose-proof-20260827/held-pose-18.mp4),
  frames 1980-2160. Inspected frames2020/2026.
- [State13 screenshot](../.analysis/owner-pose-proof-20260827/held-pose-13.png)
  and [181-frame video](../.analysis/owner-pose-proof-20260827/held-pose-13.mp4),
  frames33620-33800. Inspected frames33660/33666.

These show held-ball poses and valid sprite composition, not a claim that
all locomotion, ball/contact gates, or camera motion are now ROM-identical.

| Remaining outside this goal | Pending instructions |
|---|---:|
| Timeout confirmation prefix `$86:844E-$8467` | 9 |
| State7 idle RNG caller adoption; older unlatched facing/caller parity | Not yet censused |
| Other shot/contact/inbound compatibility callers | Not yet censused |
| Timeout/period orchestration, quarter setup, substitutions | Not yet censused |

`build.ps1 -Test` passes the full suite, including unchanged intro/menu/Player
Lab screenshot anchors and 63,800 CPU frames (145 selectors, 33 made-run
updates, 2,015 exact-pass frame checks, 85 automatic unlocks). The separate
400,000-frame runtime probe additionally checks resource validity and `+$B0`
telemetry binding. Full-suite log:
`.analysis/owner-pose-proof-20260827/full-regression.log`.
