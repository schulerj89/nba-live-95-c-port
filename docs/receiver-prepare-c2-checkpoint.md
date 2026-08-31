# C2 receiver child and AF66 parent checkpoint

Base `facd818e6e798f6546c00aeaa5dcbb84835a4cf4`, after accepted C1. This checkpoint implements `86:B468-B624` and the already-selected `86:AF66-AFA3` parent as typed C. It does **not** yet wire ordinary C gameplay, migrate shared actor aliases, or complete C2. The actual graphics publication-ring input and preceding AD3D gate remain runtime integration prerequisites, detailed in `receiver-graphics-queue-dependency.md`.

## Source implementation

`nba_receiver_prepare` implements selector/variant rolls and retry gates, the B7D8 point1 pose lookup, AA6A accuracy/error, optional B0E2 selector7 offset, F8D9/F867 signed32/16 division and final velocity/baseline/direction/magnitude/flags/global/selector stores. It retains full two-word coordinate fractions. `nba_receiver_pass_prepare` sets the pass-band-derived receiver timer, calls that child, installs receiver mode14 and passer flag4, and restores DP51. The API never reassigns actor identities or ball ownership. Source-invalid host inputs return false without changing state.

Original quirks retained with source comments include DBR:$012C instead of receiver+A8; inherited passer E0/C2 profile/statistics while96 selects receiver; wrapped CMP/N comparisons; selector1 gating on original DPB2; band30 reading opcode word8EA6; low-byte timer denominator; wrapped quotient/remainder semantics including zero divisor; and exact RNG order. B7D8 has no RNG call: AA6A/AAE1 is the intervening consumer. No arbitrary owner reassignment, clock/RNG reset or old complete initializer is used.

The child is bounded to binary arithmetic,16-bit A/X/Y and axis88>>1 in0..7 with supported animation-asset lookups. Full CPU flags/stack, interrupt timing, unlisted volatile bytes and whole WRAM are not claimed. The probe checks20 actor words plus34 global/DP/math words per child; the parent additionally checks passer flags and receiver mode. Animation+30/+32 are not installed by B468 and are not invented by this implementation.

## Fresh original evidence

Canonical ROM SHA256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`; Mesen `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`; unchanged264-entry asset pack `f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`.

The two original captures were made sequentially with isolated Mesen executable/settings/home/saves, a clean NBA95 environment and ordinary menu/directional/held/pulsed B input only. No WRAM, CPU, ROM or save-state injections occurred. They remain in the earlier worktree's **new ignored C2 folders**, not inside the C1 freeze: `c1-pass-interruption-20260831/build/c2-native-left-v1` and `c2-native-right-v1`.

| Selection | Court frames | Human pass entries | B468 calls | AF66 calls | Full-WRAM boundaries |
| --- | ---: | ---: | ---: | ---: | ---: |
| left0 | 6000 | 34 | 3 | 2: oneCPU, onehuman | 82 |
| right2 | 6000 | 39 | 5 | 0 | 118 |

The native left human AF66 atcourt5550 retains passer3CEB/C2=8/E0=ADB08E while96 temporarily becomes receiver3AEB. Timer/B2=61, DBR=7E and012C=0200. The CPU AF66 atcourt2082 similarly retains passer C2=4 while receiver96=34EB. Right provides ordinary shot-child coverage only. All eight child calls select7 naturally; other selectors below are controlled coverage. Rightcall1 crosses one observed NMI, but its checked owned words still match. That does not establish full interrupt equivalence.

Fresh Ghidra65816 and separate recompiler output in `build/c2-reference-v2` cover all reached child/parent routines and the two graphics-ring writers, plus actual selector/error/band bytes. Referencev1 is retained but omitted B0E2 from its declared listing ranges, so v2 is normative. The short producer-attribution capture `build/c2-native-012c-v1` retains a known convenience-metadata defect; its complete write log and original instructions establish the graphics queue relationship. It is excluded from the certified comparison corpus.

## Verification

- Fresh8-source MSVC `/W4 /WX` build: `build/c2-child-build-v4/hud_native.exe`, SHA256 `3a2091fb20bfc40799b401960193677efb84f18b099760463d0c15cb0a4185cc`. Build manifest binds every compiled source and all headers. No old objects or hidden post-state callback are used.
- `build/c2-final-native-v1/report.json`: eight before-only child calls,432 checked words, no mismatches.
- `build/c2-final-parent-v1/report.json`: two before-only AF66 parent calls,112 checked words, no mismatches. Input is AF66 entry; no child exit supplies the parent's inputs.
- `build/c2-final-controlled-v1/report.json`:512 source-dataflow vectors/27,648 checked words, all eight selectors; timer/division/fraction/sign/variant/alternate-pose/modifier/hot-team/stamina cases. The Python expectation reads original ROM tables and uses separate bitwise long division. This is controlled source comparison, not an independent audit or new natural coverage.
- `build/c2-integrity-v1/report.json`:29 metadata/output mutation cases refused and42 C input/asset-domain no-mutation guards passed. The verifier deliberately accepts **only the two pinned capture manifests and parsed datasets**. It is not a generic attester for new capture versions; new corpora require explicit source/route review and new pins. Exact artifact closure, command/environment/isolation/post-settings/save identities, raw/CPU domains and complete canonical C output/asset-loader stderr are checked. The known producer-attribution metadata issue is not waived into that corpus.

Earlier buildv1's ordinary C compile error (`void *` arithmetic) is retained; explicit byte views fixed it in buildv2. No native arithmetic mismatch has been found in the tested child/parent domain. Old build/replay attempts remain separate from finalv4 bindings. No native golden or existing production source was changed.

## Rebuild and replay

Run `python -B tools/build_receiver_prepare_probe.py --kind native --output NEW_BUILD`. Then use `tools/verify_receiver_prepare.py --capture ORIGINAL_CAPTURE [--capture OTHER_CAPTURE] --exe NEW_BUILD/hud_native.exe --pack PACK --rom ROM --output NEW_OUTPUT`; add `--af66` for the already-selected parent. Use the left capture for the parent; right has zero parent calls and cannot be relabeled a parent pass. `tools/test_receiver_prepare_source.py` and `tools/test_receiver_prepare_integrity.py` take explicit paths; exact successful commands are retained in their reports and the checkpoint handoff.

No normal CPU/human runtime or full-game test is presented as validation of this dormant module. Existing CPU behavior is unchanged because the production source manifest and tipoff caller are untouched. Required next work is the source-owned shared graphics ring, the actual catch gate/pointer/ordered-list composition, then production AF66 wiring and its actual actor-loop regression. Shared+56/+58/+60 consolidation follows that producer proof; whole human controls and period restart remain separate work.
