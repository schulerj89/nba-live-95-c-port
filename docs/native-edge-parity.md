# Native edge-contract checkpoint — 2026-08-29

The scoped differential gates pass, but this is not whole-game parity. New
controlled Mesen evidence proves actor rectangle responses, owned/ownerless
out-of-bounds selection and the dynamic inbound-side gate. The owned-ball
driver has **323 complete native projections and one partial case**, not 324
whole-function proofs. C-only endurance and image hashes remain integration regressions, not
independent ROM oracles.

All captures use the US ROM SHA-256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The authoritative outputs are native Mesen entry/exit snapshots. Ghidra and
generated recomp explain the paths; generated recomp is neither the fixture
oracle nor production emulator plumbing. Capture controls change documented
WRAM at naturally reached entries, never ROM, PC, stack, flags or RNG.

## Evidence and coverage boundary

| Subsystem | ROM/Ghidra and recomp | Differential result | Instruction credit / remaining gap |
| --- | --- | --- | --- |
| Actor rectangle and diagonal response | `$85:96B5-$9A13`; `bank_85_963D_M0X0` |56 controlled calls,19 outputs,0 mismatches;990 live and500 base calls still pass |224 distinct enclosing PCs observed; only66 newly witnessed edge starts /172 bytes added. RNG/event producer and special-mode integration are excluded. |
| Owned/ownerless OOB dispatcher | `$87:92A5-$949E`, captured92A8→94A2; `ViolationOobParent_M0X0` |46 controlled calls,37 outputs,0 mismatches; existing10-case parent fixture retained |Augments the existing parent row, no new blanket range or whole-parent instruction claim. |
| Inbound receiver side gate | `$86:F61F-$F647`, exitsF648/F653 within existingF60B-F668 row; `InboundSideGate_M0X0` |40 controlled calls,24 allow/16 reject,0 mismatches |Dynamic context sign proved for both actor groups. No new full launch, human inbound or dormant free-throw claim. |
| Formation context anchors | `$85:ADF5-$AE1A`, `$85:AE1F-$AE32` withinAD6B-AF5B; `FormationRoute_M0X0` |32 new controlled +64 retained cases,61 outputs,0 mismatches |Existing formation row strengthened; reversed anchors and mirrors covered without new address credit. |
| Owned ball wrapper/core | `$85:9A37` throughA7C7, raw entry9A24; `bank_85_9A24_M0X0` |323 complete +1 partial native case;431 complete +1 partial replays after host-binding variants |Only21 wrapper starts /51 bytes at9A37-9A69 added. Aggregate9A24-A7C7 has `coverage_credit:false`; owned rim/script classification and event timing remain incomplete. |

Exact artifact paths, raw/recomp hashes and the machine-readable disjoint
ranges are retained in `verified-routines.json`. Full census/progress reports
must be regenerated after this ledger changes; this document does not assign
an overall completion percentage.

## Actor integration: an edge is not an unconditional position clamp

The old990 live calls had entry X33..386/Y-183..151; the500 base calls had
X-104..104/Y-83..83. Neither corpus has a stationary outside axis or a
post-integration rectangle-edge candidate. Their earlier success did not
establish the edge rules.

Native `$85:9791` onward integrates/clamps each planar axis only when that
axis velocity is nonzero. Caps are X±394 and Y±224; negative equality also
enters the edge response. Clamping replaces the integer word and retains the
full16-bit fraction. Mode8 still clamps the integer, but retains velocity and
timer`+$60`. Other modes clear timer60 and cancel only an outward velocity;
inward velocity survives. A0 records the native edge/corner table result.
The later diagonal correction adjusts only X's integer word, regardless of
whether planar velocities were zero, and preserves fraction/velocity/timer.

Concrete captured examples:

- Case1: stationary X403:`12EF` remains403:`12EF`, timer`0123`, A0=0.
- Case2: mode11 X393:`F2EF`, VX`0123`, dt2 becomes394:`38EF`, VX0,
  timer0, A0=7. Case22 is the same mode8 crossing with VX/timer retained.
- Case5: mode11 outside X403:`12EF`, VX`FF80` becomes394:`12EF` while
  retaining inward VX`FF80`, clearing timer and setting A0=7.
- Case41: a moving positive corner first reaches394,224 and then diagonal
  X337,224, preserving fractions`12EF/34CD`; A0=6.
- Case49: stationary403,230 becomes331,230 by diagonal correction alone;
  fractions and timer survive and A0 remains0.

`tests/fixtures/actor-commit-edge-witnesses.json` contains all56 native inputs,
19 expected output words and per-call executed PCs. The56 cases cover both
axes/signs, modes11/8, five stationary/crossing/equality/inward categories,
moving corners and stationary diagonal corners. The raw source is
`.analysis/actor-commit-edges-native-20260829-v3/actor_commit_edges.vectors.jsonl`.
The fresh byte-backed Ghidra listing is
`.analysis/actor-commit-clamp-audit-20260829/actor_commit_clamp_bank85.txt`;
the readable recomp is
`../NBA-Live-95-Recomp/.analysis/recomp_gameplay_extract/generated/bank85_v2.c`.

Only these operand-inclusive edge spans receive new credit:
`859962-859983;859987-8599DC;8599E0-859A13`.
Unexecuted zero-velocity JMPs9984-9986 and99DD-99DF are excluded. The old
movement-vector row also now excludes98F4-9905: that six-instruction RNG/event
producer is outside the pure helper. An unchanged facing result cannot prove
RNG consumption or event`$13E7` bit40.

The portable ordinary common-commit adapter no longer receives a second
unconditional host clamp. Special-mode executors still using separate
integration retain a compatibility guard; their common-prefix scheduling
must be ported and natively captured before claiming the same integration
contract. The controlled entry is96B5, after the963D flag/three-second prefix;
it does not prove that prefix, landing callbacks or stochastic98F4 behavior.

Tools: `capture_actor_commit_edges.ps1`, `mesen_actor_commit_edges.lua`,
`normalize_actor_commit_edges.py`, `actor_commit_vector_probe.c`,
`verify_actor_commit_vectors.py`.

## OOB: ownership changes both the predicate and the coordinate source

At `$87:9340` the owned branch tests the owner's integer position, not its
velocity: X>=378 or X<-378, then Y>=208 or Y<-208. It retains the native
grounded/live-state guards. The ownerless branch tests the ball, requiring
nonnegative velocity at a positive edge and negative velocity at a negative
edge. X is prioritized: an outside-X ball moving inward returns without
testing an outside Y. Both captured corner controls demonstrate this early
return, including X378/Y208 with VX<0/VY>0 and X-379/Y-209 with VX>=0/VY<0.

An accepted OOB clears **both** ball planar velocities at93BB/93BE. The owned
predicate still snapshots the ball's integer X/Y, not the owner's position:
controlled case1 has ownerX378 and ball13,-17, and the native source snapshot
is13,-17. Rounding subpixels, substituting the owner's coordinates, widening
the arrival box, or exempting an inbound carrier from native actor bounds
would conceal a different first divergence.

`tests/fixtures/violation-oob-witnesses.json` retains46 rows, including both
actor groups, ownerless cases, signed equality, inward/outward/zero velocity,
airborne/live82 guards and the two X-priority corners. The capture runs
92A8→94A2 after input control92A5 and includes native9B38/9B41 effects. The
raw source is `.analysis/violation-oob-native-20260829-v2/violation_oob.vectors.jsonl`;
Ghidra is `.analysis/cpu_gameplay_ghidra/cpu_gameplay_bank87_listing.txt`;
fresh recomp is `.analysis/violation-oob-reference-20260829/bank87.c`.

C37D finalization is after this parent boundary and before deferred foul
consumption. Its native integer source, selected play and request publication
must remain distinct from the pending94A2 event. These46 rows do not turn
that later event pipeline into an isolated oracle.

Tools: `capture_violation_oob_matrix.ps1`, `mesen_violation_oob_matrix.lua`,
`normalize_violation_oob_matrix.py`, `violation_parent_vector_probe.c`,
`verify_violation_parent_vectors.py`, `regenerate_violation_oob_reference.py`.

## Inbound identity, target and dynamic side

Three different contracts must not be collapsed into host possession:

- At `$86:CFA0-$CFDE`, live82 ownerless contact with transfer9B8=0 and
  free-throw978=0 uses play996. For play<6 it checks actor`+$6E` against952
  and publishes the contacting actor to954 atCFCC-CFCF; play>=6 requires
  the candidate already equal954. BAA2 does not own this954 rewrite.
- `$85:C37D/$C477/$C579` computes an integer target with its diagonal rule,
  not a generic rectangle clamp. A positive X403 target can legitimately
  survive that helper. `$86:F4F2-$F51F` retains the asymmetric arrival box
  [-9,+8]; widening it is not an evidenced fix for an upstream stall.
- `$86:F61F` reads the live context pointer9E and context`+$0A`. For the
  captured court-coordinate domain, nonnegative anchor allows ownerX<-20
  or receiverX>=0; negative anchor allows ownerX>=20 or receiverX<0.
  Immutable actor group/initial court direction cannot substitute for it.

Natural954 rewrite evidence remains in
`.analysis/func-vectors-collision-sweep-long-20260826/collision_sweep.vectors.jsonl`:
call434/frame4753 has play1, ownerFFFF→3 and9542→3. Call696/frame5282 has
play16, ownerFFFF→7 and954 unchanged7; call1208/frame6326 similarly has
play11. These are enclosing sweep witnesses, not new isolated CFA0 fixtures.
Rejected enclosing sweeps alone do not prove a specific CFD4 rejection.

The final 63,800-frame CPU regression exposed a second caller bypass at
frame 58,614: inline scoring changed live state to82, but the ordinary
REBOUND arm's generic geometric pickup installed actor1 while leaving954=2.
`$86:CF20/$CF91/$CF98` enter CFA0 regardless of the host dispatcher label.
The generic loose-contact helper now applies that same gate after geometry,
preserving its descending-shot exclusion and 30-Hz cadence. The runtime
selector/owner assertion was retained, not relaxed. This is an integration
correction backed by the existing native enclosing sweeps above, not a new
isolated native fixture or additional instruction-coverage credit.
The fresh retained C trace at
`.analysis/native-edges-release-final/cpu_gameplay.jsonl` first differs from
the prior trajectory at exactly frame58,614: owner1/selector1 replaces
owner1/selector2. It reaches ready state by58,700, a launched transfer by58,800,
and live possession by59,000. Every earlier row is byte-identical, including
the five RGB golden-frame checkpoints. Seven startup checks additionally
exercise real ROM pose geometry, wrong-side scan continuation, early selector
publication, late-play accept/reject, geometry miss, live bypass, descending
shot rejection and odd-tick rejection. These are C integration checks.

The side-gate fixture `tests/fixtures/inbound-side-gate.json` is a controlled
40-case matrix: actor groups0/5 × anchors±336 × ownerX{-21,-20,-19,19,20}
× receiverX{-1,0}. It captures actual F648 allow/F653 reject boundaries,
not the subsequent launch. Raw source:
`.analysis/inbound-side-gate-native-20260829/inbound-side-gate.jsonl`.
Ghidra: `.analysis/cpu_gameplay_ghidra/cpu_gameplay_bank86_listing.txt`.
Fresh recomp: `.analysis/inbound-side-gate-reference-20260829/bank86.c`.

Tools: `capture_inbound_side_gate.ps1`, `mesen_inbound_side_gate.lua`,
`normalize_inbound_side_gate.py`, `inbound_side_gate_probe.c`,
`verify_inbound_side_gate.py`, `regenerate_inbound_side_reference.py`.

## Formation anchors survive halftime without swapping actors

The same mutable context source is required outside receiver selection.
`$85:ADF5-$AE1A` chooses ordinary formation sign/mirroring from context`+$0A`;
`$85:AE1F-$AE32` sends the special9A2 cutter to that full signed anchor and
Y0 while clearing its formation-install latch. Actor slots and groups do not
swap when halftime reverses the anchors. Using an initial group-side constant
can therefore put every eligible receiver on the wrong side after halftime.

The32 new genuine AD6B entries cover slots2/7 × anchors±336 × mirror0/1 ×
ordinary plays0/8/14 or the selected special cutter. Each compares61 outputs:
the route result and all ten actors' targetXY, flags7E, velocityXY and boost72.
`tests/fixtures/formation-route-witnesses.json` v2 retains the old64 witnesses
and adds32 lossless baseline/patch rows:96 cases,88AF5B/8AD77 exits,0 mismatches.
No instruction range is enlarged merely because these witnesses were added.

Raw source: `.analysis/formation-anchors-native-20260829-v2/formation_anchors.vectors.jsonl`.
Ghidra: `.analysis/gameplay85-closure-ghidra/gameplay85_bank85_listing.txt`.
Fresh recomp: `.analysis/formation-anchors-reference-20260829/bank85.c`,
`FormationRoute_M0X0`. The probe hydrates both live anchors46F5/4775. Capture
controls restore WRAM after the recorded return and leave CPU/ROM/RNG alone.

Tools: `capture_formation_anchors.ps1`, `mesen_formation_anchor_cases.lua`,
`normalize_formation_anchor_vectors.py`, `formation_route_vector_probe.c`,
`verify_formation_route_vectors.py`, `regenerate_formation_anchor_reference.py`.

### Final inbound override witness and trajectory-test correction

The fresh `formation-override-witnesses.json` adds ten separate genuine calls:
eight positives (plays 6–9, both actor groups) and two target-sign controls.
All 61 formation outputs match the existing C route. `$85:AE39-$AE56`
requires live state 82, play 6–9, nonnegative inbound Y and negative inbound X.
The team-role scan at AE58-AE84 excludes the provisional inbounder and human
controllers; AE8B/AE91 then writes literal target -40,+160 to the selected
teammate. The positive captures select actor 4 or 9. Excluded-role scan
continuations, human skipping, exhausted scans, and live/play-range rejection
are not covered by these ten native cases.

The long C-only test previously considered only the ordinary formation table,
and first rejected this valid override at frame 44729 (again at 44775).
It now checks the exact predicate/selection rather than accepting any target.
No production code or native expected output was changed for this correction.
Capture/normalization/verification tools are `capture_formation_override.ps1`,
`mesen_formation_override.lua`, `normalize_formation_override_vectors.py` and
`verify_formation_override_vectors.py`; the latter also runs strict fixture
mutation controls with `--self-test`. No additional instruction credit is
granted by this supplemental witness.

The camera assertion also now retains the native pre-wait resolver explicitly.
At global frame 52438, the pending actor-9 copy resumes after the first-quarter
break (raw period 0→1); the preceding frame's cleared ownership is not its input.
The revised guard checks the retained pointer, all four copied XY words, the
post-copy ownership resolver, and no-copy behavior during waits/freezes.
Existing 1,133 native camera witnesses and the production latch probe remain
separate oracles. This is not a claim of full native period/NMI timing parity.

## Ball dispatch: raw ownership is authoritative; call181 remains partial

`$85:9A37` branches on signed093E regardless of a host ATTACHED/SHOT/PASS
label. Negative ownership enters the shared core. With a valid owner,
actor`+$2A` belowF0 also enters the core; the high-resource wrapper preserves
the ball in modes15/17 and otherwise calls B649/B66A for position only.
Attachment uses the093E actor, not a stale handler or provisional954.
Integer XYZ replacement preserves the ball's fractions, velocities,
attachment latch and controller assignments. It is not a possession award.
The expanded31-output projection also compares both native snapshot words:
high-resource projection stores the pre-attachment ball integer X in0922
at`$87:B651-$B654`; modes15/17 preserve0922. All high-resource paths preserve
0924. Low-owned integration instead stores the pre-substep ball integer Z
in0924 at`$85:A59A-$A59D` before committing new Z, then stores the owner
integer X in0922 at`$85:A5C2-$A5C7` before composing XY. These writes apply
to both reset/fall phases and execute in both substeps. The wider projection
exposed previously omitted snapshot writes; current production matches them.

The324 retained natural records comprise66 high-resource projection,
42 high-resource mode15 preservation,85 low-reset and131 low-fall calls.
Mode17's comparison is executed, but its taken preservation branch is not
natively witnessed here. High-wrapper host-cache perturbations expand108
native cases to216 binding replays, including108 poisoned0922/0924 negative
controls for replacement versus preservation; these extra rows are C
integration checks, not extra native captures. Low-owned witnesses are restricted to
captured Z<73 and live0/82. The normalizer rejects unsupported low fractional
bits rather than inventing an oracle for the port's unrepresented precision.

The raw500-call source enters9A24 and exitsA7C7. Normalization advances only
the094A counter prefix to describe9A37 inputs; it preserves native expected
outputs. The raw source is
`.analysis/func-vectors-ball-driver-20260826/ball_driver.vectors.jsonl`.
The durable fixture is `tests/fixtures/ball-driver-owned-dispatch.json`.
Ghidra is `.analysis/cpu_gameplay_ghidra/cpu_gameplay_bank85_listing.txt`;
recomp is `bank_85_9A24_M0X0` in the same bank85_v2.c noted above.

Call181 crosses frames4241→4242. Its floor-bounce producer atA582/A585/A588
sets event13E7 bit0, but the recorded return has13E7=0. The interrupt audio
path80:8576→82:F8B1→82:FD65 can clear the bit atFD7A/FD7D before80:859B RTI.
No per-write PC was captured, so this explanation remains an inference.
The fixture retains raw0, the verifier reports PARTIAL and excludes exactly
that field, and a separate static-producer assertion checks C=1. It is not
permitted to repair the oracle or count this as an exact native event replay.

Only wrapper9A37-9A69 receives new instruction credit. The aggregate
9A24-A7C7 is explicitly non-credit, and the older ownerless/attached subrange
rows are not broadened by this checkpoint. Owned rim classification,
complete scripted/free-throw resolution, cross-frame event-consumer timing,
and full ordinary/special actor scheduling remain separate work.

Tools: `normalize_ball_driver_owned_vectors.py`,
`ball_driver_owned_vector_probe.c`, `verify_ball_driver_owned_vectors.py`.

## Reproduction and release boundary

### Reviewed C-only integration changes

The corrected native contracts change deterministic gameplay trajectories;
they do not merely change a test checksum. Before updating any C golden, the
five gameplay BMPs at frames 600, 1300, 3480, 6932 and 6954 were regenerated
and visually inspected (`build/native-edges-*.bmp`). Court, ball and uniforms
remain ROM-derived and intact. Wider loose-ball framing and occasional
player/HUD overlap remain presentation gaps; these images are not compared
to matching native whole frames and do not establish pixel parity.

The Mode-1 frame-1000 winner census was also inspected. It now has BG1 2,176,
BG2 38,703, BG3 5,641, OBJ 2,573 and backdrop 8,251 pixels: 75 OBJ pixels
replace BG2 relative to the retired C trajectory. The compositor did not
change. Its independent source/priority/palette tests remain required.

The reviewed 16,000-frame C scenarios retain independent semantic guards:

| Teams | State/render digest | Ownership changes | Score changes | Dead-ball recoveries | Resource changes |
| --- | --- | ---: | ---: | ---: | ---: |
| 0 / 18 | `5c699ab7906a9264` | 192 | 11 | 15 | 35,873 |
| 18 / 3 | `e21e3fa411911e5f` | 189 | 9 | 15 | 36,870 |
| 26 / 8 | `310e2241d5021888` | 183 | 8 | 14 | 36,112 |

Each also has 132 changing sampled renders and sustained motion on both
teams. The full UI/gameplay closure is repeatable across two runs: digest
`773c1df2a9820701`, eight handoffs, 65 render changes, 2,910 motion frames,
13,122 resource changes and 72 possession changes. These are C integration
results, not native match-score or scene-pixel oracles.

The 63,800-frame tip-flow probe reaches raw period 5 with 39,594 post-restart
live frames and no over-limit inbound stall. Its deterministic canceled-
transfer native projections and separate whole-update binding are documented
in [inbound-cancel-recovery-differential.md](inbound-cancel-recovery-differential.md).
The native clear/retain branch is mandatory even when this long trajectory
contains zero natural cancellations. Period/live/stall thresholds were not
relaxed.

### Native replay commands

After building the current probes, run:

```powershell
python tools/verify_actor_commit_vectors.py --vectors tests/fixtures/actor-commit-edge-witnesses.json --probe build/actor_commit_vector_probe.exe
python tools/verify_actor_commit_vectors.py --vectors .analysis/func-vectors-actor-commit-live-20260826/actor_commit_live.vectors.jsonl --probe build/actor_commit_vector_probe.exe
python tools/verify_actor_commit_vectors.py --vectors .analysis/func-vectors-actor-commit-20260826/actor_commit.vectors.jsonl --probe build/actor_commit_vector_probe.exe
python tools/verify_violation_parent_vectors.py --vectors tests/fixtures/violation-oob-witnesses.json --probe build/violation_parent_vector_probe.exe --pack build/nba95_assets.pak
python tools/verify_inbound_side_gate.py --vectors tests/fixtures/inbound-side-gate.json --probe build/inbound_side_gate_probe.exe
python tools/verify_formation_route_vectors.py --vectors tests/fixtures/formation-route-witnesses.json --probe build/formation_route_vector_probe.exe --pack build/nba95_assets.pak
python tools/verify_ball_driver_owned_vectors.py --vectors tests/fixtures/ball-driver-owned-dispatch.json --probe build/ball_driver_owned_vector_probe.exe --assets build/nba95_assets.pak --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc'
```

The latest scoped replay has zero unexpected mismatches, with the explicit
one-field partial exception above. Current canonical-suite and release
outcomes are recorded in `STATUS.md`; the strict configured-start differential
remains `INITIAL_STATE_MISMATCH` (62 of 449 words at baseline). No C golden was
changed to manufacture a native pass. Native captures remain evidence only;
production graphics/audio continue to use ROM-derived packs.
