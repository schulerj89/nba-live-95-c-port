# Period restart helper v2: corrected opening transform and caller domain

`nba_period_restart_v2.c` implements the gameplay state writes in `$86:DDA4–E183`, the opening/overtime branch through `$E1AC`, and the narrow children identified below. It fixes the missing source ownership needed for a regulation restart: formation actor selection, layout-0 target publication, ball initialization, cancellation, and inbound owner/mode/camera publication. It is **not wired into production** and is pending independent audit. This is not a complete restart, animation, frame-timing, or Rules reentry parity claim.

The API uses named integer/fraction words, ten typed actor records, a typed ball record, and explicit parent/child boundaries. It has no ROM interpreter, native-memory loader, delay, timing prediction, or snapshot initialization. Its diagnostic probe can accept labeled typed native child returns to compare isolated parent segments. That option belongs only to the probe; the C helper never reads a native capture.


This v2 packet supersedes the rejected v1 implementation without changing any v1 file. The independent audit found that the positive-anchor opening branch also negates Y at `$86:DDE7�DDED`; v1 C and its source-only expected result both incorrectly retained Y. Its native overtime fixture used the negative anchor, so that success did not witness the faulty branch. This was a port error, not an original-game quirk. The v2 source adds the missing Y negation, and its expected coordinates now use a separate fixed-source-block dataflow reference reading the actual ROM operands and table addresses.

The same audit found missing native caller-domain checks. The v2 header and verifier explicitly require 16-bit M/X, binary decimal flag D=0, direct-page base zero, and at `$DD97` X=0, Y=$34EB, scratch `$B6` equal to context0's carried anchor, and cursor `$9A=$34D3`. A is not pinned: `$DD97` overwrites it. These are source caller preconditions, not new initialization writes. Original ready/dead-coordinate preservation and all excluded children are unchanged. Compile only the v2 source/header, not both v1 and v2 definitions.

The retained independent rejection is `completion-auditor/docs/completion-period-restart-independent-audit.md`, SHA-256 `a1c9ce94684f997b860e33be1123aeacb73658f158f4d3f33f016104a3a4cd17`.

## Original caller and preserved stale state

The continuing-period path increments `$0926` at `$87:976E` and jumps from `$87:9797` to `$87:8C86`, reaching `$87:8CA6 → $86:DCA6`. It bypasses the new-match `$87:8C6B–8C7C → $86:DA18` path. `$DA3F–DA47` clears words `$08FC..0A09` on that separate path. Importing that clear into the period restart would change original behavior.

In particular, the helper preserves `$09BA` and the dead-ball coordinates `$09B0/$09B2`. `$85:C37D` does not clear them. The native corpus witnesses naturally carried `$09BA=1` through the complete observed period formation; the separate ready-0 case remains 0. The original explicit clear `$85:9405` belongs to its gated foul-consumption path. The original ready publication is `$86:F577`. No generic inbound finalizer that clears ready or dead-ball coordinates should be called as a replacement for these period writes.

Before this helper, the caller must still own `$DCA6–DD97`: earlier actor/global reset fields, clock selection, `$DD47` play fields, `$DD56–DD75` period-2 anchor reversal, negative initial owner/assistance, and the initial object-list setup. The helper establishes the source-fixed `$DD89` list cursor `$34D3`; this does not substitute for the remaining predecessor work. Inputs are the already incremented period, the original tip winner (`0` or `5`), and the current team anchors after any period-2 reversal. The bounded period domain is `0..4`; later overtime values are not accepted by this API.

## Formation bytes and transformations

The actual unheadered ROM is `F:\Games\SNES\NBA Live 95 (USA).sfc`, SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`. The tables occupy `$80:D06A–D0E1` (file `$506A..50E1`, 120 bytes), SHA-256 `dd60de17913ba33287e59ba165ec82714d6c5b39a842f136fd2aaf52802d9bb2`. The fourth word in each record is retained as provenance but is not read by these coordinate paths.

| Table | Five `(x,y,direction)` records |
|---|---|
| Opening `$D06A` | `(8,3,6)`, `(-16,-83,0)`, `(-24,80,4)`, `(104,-56,7)`, `(96,59,5)` |
| Regulation A `$D092` | `(110,9,6)`, `(94,131,6)`, `(-335,53,2)`, `(-300,68,6)`, `(-306,-73,0)` |
| Regulation B `$D0BA` | `(63,3,6)`, `(51,146,6)`, `(-387,56,2)`, `(-324,72,6)`, `(-336,-70,0)` |

`$DD97` selects opening for period 0 or at least 4, and the regulation rows for periods 1–3. `$DDFD–DEA4` supplies this regulation selection, independent of current anchor signs:

| Period | Tip winner | Team 0 / team 1 table | Negate both X/Y and rotate directions by 4 |
|---|---:|---|---|
| 1 | 0 | A / B | No |
| 1 | 5 | B / A | Yes |
| 2 | 0 | A / B | Yes |
| 2 | 5 | B / A | No |
| 3 | 0 | B / A | No |
| 3 | 5 | A / B | Yes |

For opening/overtime, `$DDD8–DDFA` negates team-0 X **and Y** and rotates direction only when team-0 anchor is nonnegative. `$DDE7�DDED` is the Y negation `A5 BA 49 FF FF 1A 85 BA`. Team 1 then negates that X and Y and rotates direction again (`$DEDE–DF24`).

`$E053` loops back to `$DDA7`, **not** `$DDA4`. Consequently the paired actor `+$A6` stores receive `0,1,2,3,4`, rather than five zeroes. This original quirk is explicitly commented in C. Early development incorrectly reset all five; the first native comparison exposed it, and it was corrected before this freeze. The old private build and failed development outputs are retained.

## Owned fields and explicit child boundaries

The exact named field-to-original-word mapping is `tools/period_restart_probe_fields.inc`: 35 words per actor, 12 ball words, 32 global words, and 12 object-list words, totaling 406. Some are carried values intentionally included to prove preservation. These names support individual `NbaTipoff` field mappings; they are not permission to replace an opaque block of gameplay state.

| Source interval or call | Helper behavior |
|---|---|
| `$DDA7–DF48` | Pair index `+$A6`, integer XY and target XY, three directions, actor IDs, group `0/5`, and context `$46EB/$476B` |
| `$DF4B–DFB1` | Clear actor integer Z, XYZ velocities, speed, `+5A/+60/+64/+72/+7A/+7E`, animation phase/accumulator/lock words `+3A/+3C/+42/+44/+46/+48`; preserve XYZ fractions |
| `$DFB7/DFBA` | Formation timer `+5C=300` for both actors |
| `$DFCB/$DFD8 → $87:AAB2` | **Excluded appearance/animation child**, yields once for each actor, ordered `0,5,1,6,2,7,3,8,4,9` |
| `$DFED–E03D` | After both children: focal distance `0` for pair 0, otherwise `120`; mode `4` for pair 0, otherwise `2`; interleaved object-list/link publication |
| `$E056–E0A9` | Initialize ball fractions and XY to zero, Z=80, velocity `(0,0,600)`, group `FFFF`, ID 10, list append/terminator; clear `$08FE`, publish `$0910=3EEB`, and set `$4933/$4935/$08F0=FFFF` |
| `$E0AC → $86:D85E` | **Excluded appearance/assignment/geometry child** |
| `$E0B0 → $86:D5DB` | **Excluded object-list sort child** |
| `$E0B4 → $86:A60D` | Included cancellation: `$0936=0`, `$0946/$0942/$0944=FFFF`, `$094A/$0948/$09B8=0` |
| `$E0B8–E0CE` | `$093E/$093A/$093C/$097E=FFFF`, `$0910=3EEB`; included negative-owner `$87:A9D0` result `$0940=0` |
| `$E0FA–E106 → $85:C37D` | Regulation side `tipWinner XOR 5` for periods 1/2, `tipWinner` for period 3; layout 0, actor `side+2`, target and play publication described below |
| `$E106–E183` | Regulation live state `$82`, timer 300, ball on actor's **actual** XY with fractions zero/Z24/velocities zero, `$0968/$09F6=24`, owner actor, actor mode11, camera side; included positive-owner `$87:A9D0` result `$0940=34EB+actor*100` |
| `$E1A6–E1AC` | Opening/OT live state `$81`; no regulation target, owner attachment, or timer rewrite |

The layout-0 child is bounded to `$85:C37D–C3BB` and `$C548–C601`. A negative selected anchor yields target `(394,-64)` and direction6; a nonnegative selected anchor yields `(-394,64)` and direction2. `$C579` leaves these points unchanged. Their X sign differs from their anchor and Y lies in `[-72,72)`, so `$C5C1` selects play/request `1` through `$C5D6` without taking the random-play path. `$C5AD–C5BD` publishes global and actor target XY. No other C37D layout is implemented.

The terminal regulation boundary is **before** `$86:E183 → BC9B`. It excludes that controller setup, `$E187 → 87:B538`, `$E18B → 87:B555`, `$E194 → 87:B3BD`, `$E198 → 87:AEC3`, `$E19C → 87:B649`, `$E1A0 → 87:B66A`, and the later role/geometry/sort work through `$E207`. The native ball reaches Z39 in a later pose/offset child, while this helper correctly ends at Z24. These must be integrated as their own source-owned operations, not hidden in a generic finalizer. Opening/OT stops at `$E1AC` before the later common children.

`advance` pauses immutably at every boundary. `resume` acknowledges an observation checkpoint or the caller's completed external child; it does not run that child. The terminal boundaries cannot resume. Work/input storage must not overlap, work internals must remain untouched, and published legal side/actor fields must remain intact between parent checkpoints. A production adapter should project named fields, execute the real excluded children at their boundaries, import only their owned results, and continue the parent. Skipping those children is supported only in the explicitly labeled source-only tests.

## Native and source-only validation

The parent froze ten controlled-expiry captures and 336 full-WRAM states in `completion-owner/build/period-restart-native-freeze-v1.json`, SHA-256 `04e4c13a1b7298b97fd72fac004e73f58cf6f2eb5bcddf0eaf389eeb404f3d2b`, with 573 identities. All original paths remain in that worktree; no capture tree was copied or modified. The parent attribution document describes the normal cold boot/menu CPU input and the six seeded expiry words. These are controlled expiry cases, not naturally elapsed full periods.

The helper gate uses the three enhanced `period-{0,1,2}-ready1-children-v2` captures and `period-3-ready1-children-v3`, all under that parent's `build/period-restart-attribution-v1`. All four have tip winner5. Their before/after snapshots bracket all 40 AAB2 calls plus the four D85E and four D5DB calls. The probe receives 12 labeled typed external returns per case, never an unlabeled 128KiB replacement. At 70 parent boundaries, all 406 typed fields match: **28,420 word comparisons**. This is component differential evidence for the parent segments only.

The native gate checks exact capture source revisions, executable/ROM identities, script arguments and settings, empty initial saves and generated-save identities, required artifact inventory, full-WRAM sizes/hashes, exact boundary order/PC/schema, field-to-WRAM equality, declared six-word seed deltas, and ready/dead-coordinate preservation. No observed field mismatch is masked. Of the typed words, first AAB2 changes the carried owner pointer; D85E does not change these represented words; D5DB changes list ordering/links. Unrepresented child fields remain outside this proof even where represented fields are unchanged.

Twenty source-only cases cover periods `0..4`, tip winners `0/5`, and opposed anchors `±336`. They use poisoned carried state and `period_restart_source_reference_v2.py`: a fixed source-block dataflow reference with no opcode dispatch or CPU/timing model. It reads the six original LDA long,X / STA dp table operands, source stride, original negation/rotation operands, and swap blocks from the ROM. Original block destinations define its six regulation paths; it does not import the C table or reuse the C swap/flip predicates. They check original reset/preservation behavior, all paired fields, opening carry-through, and inbound target/ownership. They do not establish native winner0 or period0 observations. Six actual malformed binary inputs and 30 parsed protocol/evidence/boundary corruptions are rejected, giving **56 passing checks** total. These include noninteger zero exit status, missing/extra terminals, the erroneous all-zero A6 behavior, ready/dead-coordinate changes, missing artifact/source declarations, altered settings/arguments, wrong hooks/register domains/raw paths, duplicate JSON keys, nonfinite numbers, decimal-mode entries, and incorrect source-carried anchor/cursor scratch.

Fresh private `/W4 /WX` build: `.analysis/period-restart-v2-build-v1`. Final native report: `.analysis/period-restart-v2-native-v1/report.json`. Final test report: `.analysis/period-restart-v2-tests-v1/report.json`. Earlier `*-dev*`, builds v1–v3, and the first new-test attempt are retained development evidence, not acceptance results. The first new-test attempt failed its mutation-reached assertion because Python equates `False` and `0`; the test now compares serialized types as well as values.

Reproduce from the scheduler worktree with fresh output names:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_period_restart_probe_v2.ps1 -OutputDirectory .analysis/period-restart-v2-build-recheck
python tools/verify_period_restart_v2.py --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-1-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-2-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-3-ready1-children-v3 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/period-restart-v2-build-recheck/period_restart_probe.exe --output .analysis/period-restart-v2-native-recheck
python tools/test_period_restart_v2.py --verifier tools/verify_period_restart_v2.py --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/period-restart-v2-build-recheck/period_restart_probe.exe --output .analysis/period-restart-v2-tests-recheck
```


The unchanged independent auditor tools also pass the new source/verifier:

- `test_period_formation_rom_audit.py`: original bounded ROM instruction execution across 20 cases, 1,600 fields, zero differences (v1 had 80 Y/target-Y differences). Report `.analysis/period-restart-v2-independent-ROM-v1/report.json`.
- `test_period_native_domain_audit.py`: all eight decimal-mode corruptions and the two wrong initial X/Y cases rejected. Report `.analysis/period-restart-v2-independent-domain-v1/report.json`.

These are diagnostic/source-only checks. The helper contains no instruction interpreter. Four native C traces and their typed component inputs remain byte-identical to v1 because those native cases do not take the corrected branch. `.analysis/period-restart-v2-preservation.json` records all 666 old v1 identities rehashed unchanged and the eight byte-identical input/trace files. Independent acceptance of this new revision is still required.

Additional source identity anchors, verified against the actual ROM rather than trusting listing instruction widths:

| Inclusive CPU address range | Byte count | SHA-256 |
|---|---:|---|
| `$86:DCA6–E207` | 1378 | `6c4f0733cbba1aaa3ba7c7c4be7a84c5369d23b1a11e454ac744764953944966` |
| `$85:C37D–C3BB` | 63 | `215d403fa82c98ced4bfd7dd69bc780d47cf5c4e71d44061e37827d4f754002b` |
| `$85:C548–C601` | 186 | `866211ad6c2afec664acbbf75fef662c59646bf0faaaa1a3ca401f94294f909d` |
| `$86:A60D–A628` | 28 | `727364ab92039b21209ae75de34dc78390c451e052ae776631124950f551e735` |
| `$87:976E–9799` | 44 | `58abf618e4d350209536270985c120b6ded7c3130a9bc2f72fc748ff82c35706` |

The read-only bank86 listing is the primary repository's `.analysis/gameplay85-closure-ghidra/gameplay85_bank86_listing.txt`, SHA-256 `836cd0cc0022f089e990650ed2b62096271632047607cef987a099901994ff90`. The ROM identity and direct table-byte verification are executable gate inputs; the listing is explanatory evidence. Independent review must precede acceptance. None of this packet authorizes a claim of production restart parity or repairs the remaining audio/SPC scheduler continuation.
