# Owner flow, unlatched reversal, and idle integration

Started 2026-08-27 from `394929e`, clean main. Requested scope is 145 decoded
instructions: 31 correction/re-verification, 22 verified-helper integration,
and 92 ordinary mode-11 caller instructions. Not 145 absent C operations.

| Slice | ROM range | Baseline pending |
|---|---|---:|
| Unlatched pose switch | 86:E545-E592 | 31 |
| Idle state7 cadence | 87:AD86-ADBD | 22 |
| Held-ball stop/facing | 86:F38A-F3B6 | 17 |
| Base matchup / pose call | 86:F3B7-F3CA | 8 |
| Owner-state flags | 86:F34F-F389 | 22 |
| Owner/dead-ball gate | 86:F3CB-F3DC | 7 |
| Decision countdown | 86:F3DD-F3F5 | 11 |
| Lost owner normalization | 86:F3F6-F40A | 8 |
| Timer reload / CPU calls | 86:F40B-F439 | 19 |
| Total | | 145 |

Baseline census: `.analysis/owner-pose-recount-20260827/shot_state_bank86.txt`
and bank87 listing. Each range decodes continuously, without data or BRK/COP.

## Plan

1. Preserve baseline; inspect focused recomp/Ghidra and label/comment mapping.
2. Independently capture full unlatched channel writes, idle RNG/resources,
   and owner-flow outputs/callee boundaries. Controlled WRAM cases must be
   labeled, never PC/stack/ROM patches. Replay all represented owned outputs.
3. Integrate reversal, state7 and ordered owner calls. Verify caller inputs,
   early returns and nonlocal CPU outcomes as well as helper results.
4. Run full regression, sustained CPU tests and inspect native frame/video
   evidence; update status/ledger accurately, rebuild, commit and push main.

## Known corrections to preserve

- E574/E586 call B37C to install the opposite dribble pose and reverse lower
  phase while restoring upper phase. The old two-output oracle missed this;
  its whole-range verified claim must be replaced with full-channel proof.
- F38A stops a held-ball actor and may write desired facing before pose
  selection. F3B9 reads base pairing +74, not current pairing +76.
- F3F6 normalizes stale owners and returns: it does not immediately execute
  mode1 behavior. F3DD uses wrapped signed countdown and preserves overshoot.
- F428 -> optional F431 -> F435 order matters. Existing child implementations
  and their separate proofs remain distinct from this caller's verification.
- Live-state82 branches to existing inbound continuation at F43A; that body,
  timeout confirmation (9 instructions), period flow and substitutions are
  outside this requested table. No human gameplay controls are added.

## Implementation and proof

All nine rows are implemented/re-verified; 145 instruction starts are covered.
`nba_owner_flow_run` in `src/nba_owner_flow.c` preserves the native ordering;
`cpu_owner_flow_call` binds it to existing runtime children. The pose and
idle helpers use asset-pack descriptors. No emulator art/audio is used.

| ROM witness | Controlled | Natural | Result |
|---|---:|---:|---|
| Full unlatched channels / facing / unchanged resources | 100 | 24 | 124 exact |
| Owner caller, including child boundaries | 44 | 1,857 | 1,901 exact |
| Idle cadence / RNG / both resources | 48 | 22 | 70 exact |
| Supplemental contact facing | 0 | 3 | 3 exact |

Total: 2,098 calls, zero mismatches; 2,095 belong to the requested table.
294 durable witnesses live in `tests/fixtures/owner-flow-witnesses.json`.
The fixture preserves natural and controlled cases separately.
`verify_owner_flow_vectors.py --require-census --listing-dir ...` proves
the captured PC sets equal the fresh Ghidra decode, with continuous ranges
and complete instruction endpoints: **31 + 22 + 92 = 145**.

Caller snapshots include 27 input/persistent words and each pose/CPU/
formation/receiver boundary. Replay compares the state BEFORE each child,
then feeds that child's recorded outputs to the caller. This verifies caller
composition, not the child body; child implementations retain their separate
ROM proofs. Native CPU nonlocal returns that do not reach the caller exit
are not misreported as completed caller captures. A separate regression
checks the ESCAPED path preserves CPU writes and skips later callbacks.

Runtime binding checks cover base +74 versus current +76, stop-before-pose,
preserving display +52, lost-owner immediate return, timer units/overshoot,
both reversal directions, resource preservation, and 100 actual dispatcher
idle-cadence ticks. `$87:8E7F-$8E9D` constructs C8=C6<<4: all 1,857 natural
owner captures have C8=32. The integration test caught and corrected an
initial mistaken C8=2 binding before completion.

Inbound owners also enter the F34F prefix before branching to the existing
F43A continuation. The old early inbound dispatch bypassed those flags/pose
writes. A runtime test now requires prefix output with an unchanged decision
timer at the live82 boundary; the continuation body remains separate work.

Two deliberately constructed negative-lock cases initially used stale
descriptor bank DP49. B3BD returns before initializing that bank when locked;
B37C then reads the stale bank, yielding garbage frame counts. Repeated
fixtures explicitly supply the valid bank84 descriptor context used by the
asset pack. C does not emulate arbitrary scratch-bank reads. The failed
experimental captures remain in `unlatched-all`; final valid-context captures
are in `unlatched-final`. No mismatch was hidden with a numeric tolerance.

The longer integration test exposed a separate contact-facing defect:
`$86:C217` and `$86:CB5E` use F02D's eight directions, while C had called the
fine pass quantizer. That could leave facing13 after a knockdown and fail
ball attachment. Both now use `nba_gameplay_contact_facing`. Three natural
C217 entries confirm both input velocities and output; CB5E has the same
Ghidra/recomp call sequence but no natural witness in this run. Neither is
added as new verified-ledger coverage for this goal.

## Reproduction and artifacts

Root: `.analysis/owner-flow-proof-20260827/` (local, ignored).

- `ghidra/owner_flow_bank86.txt`, `owner_flow_bank87.txt`: fresh decoded
  census. `DumpOwnerFlow.java` saves named labels and C mapping comments in
  the existing bank86/bank87 Ghidra projects.
- `.analysis/owner-flow-recomp-20260827/generated/bank86_v2.c` and
  `bank87_v2.c`: focused regenerated exact M0X0 bodies. Emulator/LLE wrappers
  are reference plumbing, not C gameplay being claimed as ported.
- `unlatched-final/`, `flow/`, `idle/`: completed headless Mesen captures.
- `contact-facing/`: completed 32,000-frame natural contact capture.
- `final-early.mp4` (frames760-1000), `final-held.mp4` (29320-29440): native
  asset-pack C output, not ROM captures used as assets.
- `final_600.bmp`, `final_1300.bmp`, `final_3480.bmp`, `final_6932.bmp`,
  `final_6954.bmp`: inspected/rebaselined C screenshot regression anchors.
  These are not claims of full-game frame parity with the emulator.
- `full-regression-complete.log`: final suite result (see completion below).
  `full-regression-final.log` passed before the final inbound-prefix binding;
  the complete log supersedes it.

```powershell
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-flow-new/unlatched -Kind unlatched -Controlled
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-flow-new/flow -Kind flow -Controlled
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-flow-new/idle -Kind idle -Controlled
.\tools\build_vector_probe.ps1 -Name owner_flow_vector_probe
python tools/verify_owner_flow_vectors.py --normalized --require-census --vectors tests/fixtures/owner-flow-witnesses.json --probe build/owner_flow_vector_probe.exe --pack build/nba95_assets.pak
.\build.ps1 -Test -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
```

## Endurance and remaining boundaries

Unforced two-team runs retain the mandatory special-shot reachability and
release guard. Chicago/Orlando: 200,000 frames, 713 selectors, score114-87,
specials selected/released at 118276/118304, 190256/190284, 198612/198640;
Orlando/Chicago: 200,000 frames, 710 selectors, score108-93, specials selected/
released at 165262/165290 and 179622/179650. Both reach held states13/18 with
valid resources. No natural state7 selection occurs in those C runs.

**Idle cadence is adopted; its upstream defensive selector is not.** Native
`$86:E39A-$E3CA` installs base7 for a stationary nearby defensive pair, called
from the wider E3E1 defensive pose flow. Those callers were not in the
145-instruction table and remain next work. Do not invent an idle randomizer
or claim the C game's idle frequency matches the ROM.

Other unchanged boundaries: F43A inbound continuation/compatibility pass
handoff, legacy dribble/contact phase consumers, timeout confirmation (9
instructions) plus period/substitution orchestration. The initial frame220
tip handoff and clock seed remain bounded scaffolds. No human controls added.

Coverage: verified captured address positions 6,917 -> 7,000,
24.79% -> 25.09%, with denominator27,901 unchanged. The 31-instruction
unlatched correction and 22-instruction idle adoption are not double-counted
as new code coverage. ADBB's range endpoint now includes its complete JMP
through ADBD; this is bookkeeping, not an additional decoded instruction.

## Completion

Final `build.ps1 -Test` completed with exit0. All ROM witness suites,
asset/scene/audio safety, title/legal/EA/menu transitions, team/player setup,
player lab/introduction, tip-off and Gameplay Lab regressions pass. The
63,800-frame CPU trace passes: 142 selectors, 28 made-run updates, 2,005 exact
pass frames and 91 automatic unlocks. Ball attachment and the >2,400-frame
dead-ball stall guard remain intact. All five reviewed screenshot hashes pass.

The old uninterrupted frame1800 clock snapshot became invalid when a real
inbound pause occurred earlier. It was replaced with stronger per-outer-frame
clock-helper binding checks in both 16,000-frame shot-state runs, including
pause/resume; the early frame220/400 clock anchors remain.

The build is refreshed; the existing desktop shortcut targets this build
and asset pack. This report and the durable fixtures accompany the main
checkpoint. Failed intermediate logs are retained locally for audit, not
presented as the final verification result.
