# Jump/reach: translated, differentially tested, and runtime-wired

2026-08-28. The normal tip-off now calls the translated decision from the
native F780/F886 behavior continuations. Its actor Z/velocity, animation
channels, contact geometry, rendering, telemetry, toss countdown and first-tip
possession are production state; there is no fitted jump/contact frame curve.

## Implemented and tested

`nba_jump_reach_decide` in `src/nba_jump_reach.c` translates the bounded
`$86:EC32-$EE75` decision: grounded/lower-pose gates, ownership/facing checks,
receiver and distance checks, rating-adjusted height threshold, RNG draw
order, vertical launch and signed planar launch. It returns ordered child
requests instead of pretending a request implements a child routine.

The final capture set has454 natural calls and197 controlled native calls.
Their651 decision replays agree on all three velocity words, RNG word and
ordered child requests. The197 controlled calls execute **all239** Ghidra
instruction starts:77 in EC32-ECF8 and162 in ECF9-EE75. This is instruction
execution coverage, not239 independently verified machine instructions.

For100 actual animation-start calls, the production
`nba_player_animation_command` API also matches all18 captured channel words:
queue cursors/contents, upper/lower/base state, phases, accumulators, locks
and upper phase target. Gate-only calls preserve these words as expected.
EAA8/BD1F child effects are explicitly excluded from that channel comparison.
Twenty natural near-branch EAA8 calls separately match all14 represented
entry/exit words. BD1F is bound with both accepted and locked-upper runtime
guards. EAA8's far lead branch still lacks a native witness and is not claimed
verified.

647 distinct native witnesses are retained in
`tests/fixtures/jump-reach-witnesses.jsonl`. The independent Ghidra census is
`jump-reach-instructions.json`. `build.ps1 -Test` now checks native replay,
eight deliberately corrupted-oracle cases, unsupported-input rejection,
the ROM data tables and all348 roster rating pairs. No C-produced values
are used as native expected outputs.

## Addresses and reference evidence

| Native routine/data | C correspondence |
|---|---|
| 86:EC32 | Entry gates and owned-ball facing decision |
| 86:ECF9 | Free-ball height/rating/threshold continuation |
| 86:ECA1 | Magnitude/RNG selects upward velocity528 or600 |
| 86:ED7C-EE29 | Wrapped signed deltas, signed divide by20, optional arithmetic half |
| 86:EE41 | Low-ball direction/movement/current-RNG gates |
| 85:F0AA | Fine facing; two equality cases differ from existing F3C3 helper |
| 80:CEE7 | Existing exact LFSR helper; zero-state recovery retained |
| 87:B3BD / B47A / B4DB | Existing animation install both / upper / lower |
| 86:EE76-EF05 | 72 literal vertical thresholds, asset280 |
| 85:F16F-F18E | 32 literal direction bytes, asset280 |

Fresh headless Ghidra named comments are saved by `DumpTipoffFlow.java` at
EC32, ECF9 and ECA1. Reproduce the bounded recomp source with
`tools/regenerate_pending_reference.py`; existing reference is
`.analysis/pending-diff-reference-20260828/bank86.c`. That generated fragment
is inspected as a reference, not executed as a second oracle.

The asset pack now has268 entries. Asset280 contains184 bytes: an8-byte
header,144 threshold bytes and32 direction-map bytes. Roster asset251 uses
previously reserved bytes29/30 for native profile3C/3D. Neither asset contains
Mesen screenshots or gameplay snapshots. The public roster lookup refuses
old packs without the new extraction rather than treating padding as ratings.

## Runtime producer/caller proof

ED32 and EDEF are the literal bytes `B9 0A 00`: LDA000A,Y. Y is003C, so they
read **WRAM0046**, not the team context in X. Recomp and Ghidra agree. That
word changes throughout the native run; hardcoding zero or substituting
team orientation would change which player rating is used.

The capture records its most recent observed writer PC. Natural witnesses
include87A620,87B836,87B847,87B40D,87B49D and82F13F (callback PCs can point
after the store; these are not claimed as exact store-opcode addresses).
This is shared projection/animation scratch, not actor+46's animation lock.
The producer lifetime is now represented by `nba_graphics_scratch_step` and
the actor projection writer in `nba_tipoff_update`.

The overlap is concrete:87A61E writes actor+28 to DP47;87B834 clears DP47;
87B845 decrements DP47;87B40B and87B49B put an upper-animation descriptor
pointer into DP47. A16-bit read at0046 consumes0047 as its high byte. These
store addresses are present in the retained bank87 Ghidra listing. Tracking
only a team-facing flag or actor+46 lock cannot reproduce this dependency.
The focused fresh headless dump is
`.analysis/jump-reach-20260828/ghidra/shot_state_bank87.txt`.

Native EC32 calls return to86F78B and86F896, proving calls atF787/F892.
The native ball stays at Z80/VZ600 while `$09F2` counts120 toFFFF, then the
shared ownerless-ball physics advances it. The C runtime now does the same.

`$82:F02F-$F15B` supplies the overlapping high scratch byte. A fresh 29-call
native trace matches all three queue records/current frames/timers, scratch46
and shared RNG after every call. Preserve this ROM compatibility quirk:
duplicate probing scans the empty slot too, so `$FFFF+6` wraps to `$82:0005`;
Mesen observes word `$0001`. A duplicate increments the candidate linearly
without another RNG draw. This behavior is flagged in the C source rather
than silently "cleaned up."

The runtime probe now observes two physical launches, first contact at C
frame164, receiver upper-reach `$37`, and final possession at frame186. Before
the CF38 receiver continuation was wired, possession remained zero after1000
frames. Native reference frames are contact185 and final possession206 in the
captured launch context; the roughly20-frame configured-launch offset is
reported, not hidden by forcing frame constants.

The parent239-start range is now eligible for the verified ledger. This does
not establish whole-game state equivalence: the configured baseline still has
unsynchronized CPU/human setup and RNG-producing callers outside this slice.
The far EAA8 child branch, complete presentation DMA, and wider initial
formation scheduler remain separate work.

## Reproduction and scope

```powershell
./build.ps1 -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc' -ExtractAssets
./tools/build_vector_probe.ps1 -Name jump_reach_probe
python tools/test_jump_reach.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --pack build/nba95_assets.pak --probe build/jump_reach_probe.exe
python tools/verify_jump_reach.py --cases .analysis/new-jump-cases.txt
python tools/run_differential.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --pack build/nba95_assets.pak --output .analysis/new-jump-native --capture-jump-reach --sweeps 120
```

Add `--jump-cases .analysis/new-jump-cases.txt` to a separate new capture
directory for controlled tests. They alter WRAM inputs at real EC32 entries
and restore saved gameplay ranges at return. They do not patch ROM, PC,
flags or stack, and are not natural-play frequency evidence. The requested
input records and actual captured inputs are separate: valid native actor
pointers and aliases are resolved by the capture adapter. Rated profiles
are redirected to a controlled WRAM copy for these cases only.

`run_differential.py` still exits1 for the separate whole-game baseline
mismatch. Check the bounded report separately with `verify_jump_reach.py`.
The pure decision status remains `DECISION_PROJECTION_MATCH`, never full-game
PASS. Runtime tests additionally cover scratch state, pose-resource adoption,
physical actor motion, contact reach and possession. CPU registers/cycles,
presentation DMA, far EAA8 and complete post-return WRAM equality remain out
of scope.

Final evidence: `.analysis/jump-reach-20260828/decision-report.json`,
`natural-v3`, `controlled-v3`, and `ghidra`. Earlier natural857-call replay
also passed, but predates the18-word channel capture and is not substituted
for the final651-call set. Release results are recorded in STATUS.md.

All release test blocks passed across `release-v2.log` (through core safety)
and `release-tail.log` (Team Select through CPU endurance and intro). The
resume followed a count-only F12 golden update, independently checked against
historical image data by `test_shot_assets.py`; no game-art baselines changed.
The final647-witness/asset/tamper test passed again after export and guard
updates. No commit or push was made in this checkpoint.
