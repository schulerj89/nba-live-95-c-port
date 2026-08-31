# Typed period formation: independent bounded acceptance

**PASS for the frozen typed DD97–E207 composition, its source closure, and its verifier.** This accepts the declared component and refusal boundaries. It does not accept production wiring, normal-state initialization, a complete period transition, CPU/interrupt/timing parity, or human play.

The scheduler freeze `.analysis/period-formation-freeze-v1.json`, SHA-256 `8265eb8e8e71e6c59186b2a0526d9ea75c02fe96a80ccf18bed5507dde41e244`, was independently checked before and after review: all **1,594 identities** match. Evidence and private source copies are under the auditor worktree's `build/period-formation-audit-v1`. No original source, fixture, dependency, or freeze was changed; no commits or enabling changes were made.

## Fresh checks

The auditor freshly compiled all 19 C translation units with MSVC `/W4 /WX`, using the frozen source/header snapshots. No existing linked objects were borrowed. The baseline build has no warnings. The 30 dependency snapshots were independently checked; the appearance/support C and headers exactly match owner commit `979c042`, as do the controller C/header. The other 22 tracked dependencies match that commit after CRLF/LF normalization only. The accepted render C/header match their separate frozen identities. The actual snapshot bytes, rather than normalized copies, were compiled and remain hash-pinned.

| Check | Independently reproduced result |
|---|---|
| Four original native captures | 125 checkpoints × 1,028 gameplay values = **128,500** comparisons |
| Frozen controlled suite | 47 cases, 1,600 original-ROM coordinate fields, 8,651 original-ROM role alias fields |
| Frozen entry/protocol suites | Eight entry refusals and 32 protocol rejections |
| Additional protocol suite | Valid baseline plus **30 rejected corruptions**; exact command and single-before input asserted on every C invocation |
| Compiled ownership/preflight suite | **83,562 checks**; 1,029 nonoverlapping C fields, 211 role aliases in both directions over 64 patterns, and explicit carried-nearest refusal |
| Omitted CPU input poisoning | 123 original-ROM executions from current C-produced E1E5 states; **25,953** owned values and terminal PCs unchanged; 192,960 source instructions across 451 PCs |
| Frozen baseline output preservation | All 12 typed-input/JSON-trace/stderr artifacts byte-identical |

The natural captures are the four previously audited controlled-expiry journeys: period seeds 0/1/2 use `period-*-ready1-children-v2`; seed 3 uses `period-3-ready1-children-v3`. They start from cold boot/controller navigation with declared expiry writes, not a claim that an unmodified match naturally reaches each sampled period at that time. C receives exactly one DD97 before-state projection per journey. Later raw states are only expected outputs.

## Canonical storage and actual child execution

The production module has no raw WRAM buffer, instruction dispatcher, native snapshot loader, after-state callback, hardware response adapter, or captured duration input. Its API accepts named fields in `NbaPeriodFormationState`, continuation work, and an asset pack. The probe's `PFC1` file is 2,110 bytes: four magic bytes and 2,106 serialized data bytes. The MSVC structure is 2,108 bytes because of ordinary padding; neither padding nor host addresses are part of the protocol.

There are 1,029 uniquely owned fields of widths one, two, or four bytes. A separately compiled layout harness checks each declared width against `sizeof` of the actual C member and proves that their C storage intervals do not overlap. It also checks all 211 role projections against the previously accepted original-address role map, without using the composition author's alias-map JSON to select expected pairings. Both load and save directions preserve every unrelated canonical field. This catches alias mistakes that an unchanged native value alone might conceal.

The source calls the actual accepted children on current C-produced state:

- Parent v2 produces each first/second appearance entry, then the composition executes `87:AAB2` before resuming the parent. Ten calls retain their original paired order.
- At E0AC, the assignment child consumes the current teams, roster slots, and selectors. All 24 carried roster addresses must match the actual asset-derived team/slot records; the module does not silently replace inconsistent carried pointers. Its complete ten-slot sort buffer is published before roles consume it.
- D5DB consumes the parent-produced 11-object list. A6, actor fractions, reset fields, and the later parent ownership writes remain owned by the parent rather than reconstructed by each child.
- Regulation E183–E1A4 executes the actual controller transfer, upper/lower cancellation, descriptor installation, pose resolution, and attachment. Complete queue arrays and shared channel phase/accumulator/lock fields are transferred, not reconstructed from cursors. Pose direction maps to actor `+52`; the input facing maps to `+4E`.
- E1AC writes predicted `0918/091A` from current integer ball XY. For opening/OT, source `E1CE` and `E1E1` transfer actor 0/context 0, then actor 5/context 1. Regulation skips these two additional transfers. The actual ROM bytes and decoded instructions are retained in `raw-rom-bridge-focal-v2.txt`.
- E1E5/E1F3 run the accepted role v2 source in context order, including B95C and RNG work where supported. The composition uses the repaired v3 verifier contract; it does not substitute the older prefix's early-return-only behavior.
- E1F7 performs D5DB, then `80:FBFF` depth sorting, then the 84A/84C increment. FBFF is the original gap sort, not a video-DMA operation or the ordinary FC80 per-frame pass.

The important aliases remain single values: actor `+60` is the parent action timer and role reaction timer; actor `+16` belongs only to controller assignments; context `+3B/+3D` belongs only to controller count/cursor; `07F6` is one RNG word; `09DA/09DE/09E2` are sort-buffer slots 0/2/4. There are no duplicate independent role latch copies. Full controller tail words, full queue arrays, roster pointers, and unrelated carried pose values remain typed and preserved.

The earlier raw diagnostic composition was useful for checking call order and source-to-field correspondence. Its reports and output snapshots do not generate this probe's commands or expected data. This review did not borrow its raw-state implementation as production storage.

## Refusals, CPU scratch, and original behavior

The earlier DCA6–DD97 entry prefix remains a caller precondition; this component does not execute it. The source domain is binary arithmetic, M=X=D=0, DP0, with valid original assets. Period/tip constraints come from the accepted parent; context links must be canonical and the leading collision sentinel must be zero. Actor IDs are not invented entry prerequisites: the parent writes them before child consumption.

`begin` and child refusal checks preserve the declared boundary contract. Successful child projections commit together; unsupported child projections do not partially overwrite canonical state. Preceding completed parent/child checkpoints remain visible. Repeated `advance` while waiting is immutable. COMPLETE, REFUSED and ROLE_STOP cannot resume; no unresolved return is guessed.

Twelve role CPU temporaries are not canonical gameplay inputs. This includes `0092`, whose old value can matter if no focal candidate beats the original `7FFF` initial best value. `roles_have_defined_nearest` explicitly requires a winner in both five-actor scans before the composed role calls. It uses the original wrapped subtraction, magnitude and swap behavior. The compiled independent test verifies no-winner refusal before any canonical change, repeated refusal immutability, non-resumability, and the correct owner-versus-prediction coordinate source.

The source's signed-wrap behavior matters here: a raw X delta `8000` is swapped by the wrapped CMP and produces distance `2000`, so it can win; it must not be treated as an ordinary positive host magnitude or assumed to fail the nearest test. A first auditor test made that incorrect expectation. Its failing `independent-ownership-v2` run is retained. The corrected v3 harness agrees with original `BC84–BCCx` instructions; no C source was changed. The earlier v1 harness compile warnings are also retained. A preliminary disassembly started inside an instruction; only the corrected `raw-rom-bridge-focal-v2.txt`, starting at BC84, is source evidence.

The 12 omitted input words were then set to three distinct nonzero patterns in 41 current-C-state role cases. Original-ROM execution still matches all 211 projected outputs and each terminal PC. This uses the already independently reviewed bounded role instruction reference, SHA-256 `965c2a188ddcf3ee0bba53299eaedb13899e02c3c74a99bf44c9eeb91bd6f0f6`; it is not a second independent CPU implementation. These controlled tests support the dead-input argument for the tested source paths, alongside the source inspection. They are not natural reachability evidence.

DP9A is different: it is a projected parent list cursor and later role pair scratch alias, but the included child APIs do not expose all CPU residue. It is the **only** field excluded from native gameplay comparisons. Its exclusion is explicit and is not expanded to hide owned differences. No CPU, DP, register, stack, interrupt, or elapsed-time parity is accepted.

Original behavior remains intact and commented in the accepted source closure: A6 receives the pair index, actor fractions survive, the original `09BA/09B0/09B2` carry survives the period route, B630 retains its default-lower-table check, paired actor help destinations are preserved, F34F retains wrapped comparisons/ASL and its diagonal edge, and sort ties/overflow retain source order. The current composition's original-ROM and native comparisons require those values; none was normalized to simplify the adapter.

BF51 record reads and BF98/related assignment children remain explicit role stops with current partial source outputs visible. Their source-only controlled witnesses do not make them natural period-path coverage. CPU-only appearance remains gated; supplying a human controller assignment refuses instead of silently skipping palette work. The typed refusal is a supported-host-domain boundary, not an assertion that the native game refuses or that an unresolved divergence is an original bug.

## Verifier and provenance review

The verifier pins the original ROM, exact asset pack, private dependency manifest and accepted native-reader identity. Its build guard requires the complete exact source/header set, integer status, current bytes/hashes and executable identity. The independent build also inspects the actual compiler invocation; a self-reported manifest alone is not the compilation proof.

The accepted native reader enforces complete hook order, duplicate-free JSON, typed numbers, exact raw-field equality, original entry X/Y/B6/9A relationships, M/X/D/DP domain on every relevant row, source/command/environment identities, declared seed writes, exact artifact inventory, completion, and private settings/save isolation. The formation verifier derives its expected C boundary sequence from the real entry period/tip, validates all output field widths on every row, and compares all 1,028 owned values at each corresponding boundary.

The additional independent protocol tool tests duplicate/missing/reordered native hooks, unknown tags, late decimal mode, wrong raw fields, entry registers, chronology, duplicate JSON keys, changed command/source/isolation/completion, wrong build identity and missing header, floating status, extra output rows/keys, byte/address overflow, wrong role metadata and stream errors. All reject; the baseline still compares 32,896 values. It asserts the exact probe executable/pack/input command and verifies that the input bytes equal only the actual DD97 before projection on every C run. This is a direct check against hidden child-after input or route changes.

## Identities and retained evidence

| Object | SHA-256 |
|---|---|
| `src/nba_period_formation.c` | `851ffd7016f594a2b5340ee9dee91204c0d444ed8d479f4a7524db58427ddb6d` |
| `include/nba_period_formation.h` | `bab18876a5c46bcbe49f0f424aea747e43266badca18708391029b3adc8b4486` |
| `tools/verify_period_formation.py` | `9a7c5d23a03aa6f871ee4059315f19f3c9f0933bf8f0a1abf7e35cdf2f12e505` |
| Diagnostic field map | `5c8cb1b47d671259d04a195bf017c1a079b5ac62143d388d9b8291bfd82ed09b` |
| Fresh private executable | `7cc5fbc784da7c11ca721f46ef954d4fa638f7f19dd8cffb5e3a31a4cb1bc91a` |
| Independent protocol tool | `9da4dbeac3abf19e8a73fc3a1bee86802775ca5aa9f760fe706e994c64630126` |
| Independent dead-scratch tool | `43980d8a1a46389bc2fc47791d8cf21555dae183aa062480bfb091f74581dd37` |
| Independent compiled ownership runner | `12d5b0ceb6c279ff1a2c29cc6d83b67448011550ba9a71fa12da271705fdbbb2` |

Relevant private result folders are `compiled-v1`, `native-v1`, `controlled-v1`, `protocol-v1`, `independent-protocol-v1`, `independent-ownership-v3`, and `independent-dead-scratch-v1`. `final-freeze-recheck.json`, `dependency-commit-check.json` and `native-output-preservation.json` record final identity checks. All earlier audit failures remain alongside the final results.

For the later adapter, draw order and basket `3FEB` XY are real carried state. Basket X already belongs to the owner's `court_presentation.basket_x_3fef`; this component does not justify a second independent owner or reconstructing basket X from a newly flipped anchor. The normal draw-list initializer, subsequent FC80 passes, frame crossings, display/HUD state, and full game integration remain outside this acceptance.
