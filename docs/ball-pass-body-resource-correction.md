# Ball/pass alignment: preserve the published body pose

Baseline integration8f5b90382b112c298cac9c02ef035cb0ab848f00; private branch work/ball-pass-alignment-20260831. This fixes a demonstrated port rendering error. No ball physics, attachment offsets, fractions, pass trajectory, controller, animation producer, actor_draw_direction, nba_gameplay_draw_direction, source manifest or native golden changes. The previously frozen C1/C2/contact packets are untouched.

The user's image matches the first CPU passer's frame306 geometry. Baseline actor3 has world(-114,104,0), published body resources332/1168, resolved facing6, selected head direction4. Its ball is(-119,99,47), projected(201,105). The old render/telemetry adapter recomputed a different body pose324/1154 using head direction4. Correct attachment to the published pose therefore appeared separated from the drawn hands. The separate D1 direction candidate changes receiver4 at this frame, not this passer or ball; it did not create this defect.

Original87:A4E1/A4E4 reads actor+2A intoD6; A517/A51A reads+2C intoD4. A52C-A5FA then selects head facing; A609 updates only status bit2 and A61E copies status toDP47. D4/D6 survive unchanged into ordinary80:AD92 and camera-subject80:AF1E. B0FF submits the already-projected ball usingDP92/8E and attributeA2+3F97; it does not move the ball toward a newly selected torso. The correction introduces one draw-only helper that returns published +2A/+2C whenever valid. Render and draw telemetry use it; the uninitialized preview retains the existing fallback. Shared actor_animation_resources and all physics callers are unchanged.

## Original evidence and scope

Canonical ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`, SHA2562115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870. Fresh linear source listings and tool identities are in `build/ball-source-v2`; v1 was retained with the earlier shorterA47A-A620 listing. No generated image or invented asset was used.

`build/ball-native-v1`: ordinary neutral CPU cold boot/menu,600courtframes/4990total; isolated private Mesen/settings/home/save folder, no WRAM/CPU/ROM mutation or injected state. MesenSHA d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b. Capture manifestSHA4f17d6675caa4ea9ab6707389b9a7e1f39ecea56c2295efddc46982906305af9. Capture process completed0 and the slot was released.

There are271 fullWRAM+CPU/master-clock records:43 complete draw groups (24AD92/19AF1E),19B0FF ball submissions and11 entry-only culled records. All eight resolved pass directions occur among complete groups. `verify_ball_draw_resources.py` checks592 source-owned words: D4/D6 and actor+2A/+2C retention at three stages, plus projected ball submission coordinates/attribute/resource. The11 entry-only groups have original+6A=FFCE and stop beforeD4 publication; this establishes their source skip domain, not observed exits. Three NMI crossings are recorded. This is native source-contract validation, not a complete C/native CPU/DP/OAM/interrupt replay. The verifier is deliberately closed to the exact immutable capture manifest and checks every artifact, private settings before the isolation helper mutates copied metadata, raw record domain/order, source bytes, and the declared owned words.

## Actual C callers and visible sequence

Fresh40-source `/W4 /WX` CLI: `build/ball-candidate-cli-v3/hud_cli.exe`. Fresh actual-source diagnostic: `build/ball-probe-candidate-v3/ball_pass_render_probe.exe`. Exact base tipoff source is separately compiled by `--baseline` in `build/ball-probe-baseline-v1`. Both execute the public actor loop and draw telemetry;116actual rendering calls cover every frame275..390 and are checked for no mutation of NbaTipoff or NbaSession.

`build/ball-runtime-final-v1/report.json`:63800frames,628226valid body-pair checks, all eight pass directions. Candidate0mismatches; baseline12466. Earliest baseline failure is frame176(actor3:244/1104 became243/1103); frame306 is the reported pass example. All63800per-frame telemetry checksums match after excluding only explicitly listed body-resource-derived draw/appearance fields. Ball/actor positions, fractions, velocities, modes, pass ownership, RNG, clock and head direction remain included. FNV is a diagnostic checksum, not a cryptographic proof; exact390-frame JSON comparison independently confirms all non-appearance fields unchanged. Source HUD unsupported-route diagnostics remain visible and identical.

Before/after production screenshots are in `build/ball-baseline-sequence-final-v1` and `build/ball-candidate-sequence-final-v1`, with exact command/commit/dirty/executable/ROM/asset manifests. The264-resource pack is unchanged SHA f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09. Named scenes:304before,306/308windup,312raise,318last-attached,320release,332flight,346receiver and350catch. At306 the candidate draws332/1168, putting the visible hands at the existing ball. At320 ownership becomes-1; at346 receiver4 owns the ball. Frame pairs and the entire275..390sequence are retained. `build/ball-comparison-v1/index.html` is a side-by-side gallery; it is an unreviewed candidate, not the published latest integration gallery.

Repeat commands:

```
python -B tools/build_ball_pass_render_probe.py --output NEW_CANDIDATE
python -B tools/build_ball_pass_render_probe.py --baseline --output NEW_BASELINE
python -B tools/test_ball_pass_render.py --candidate NEW_CANDIDATE/ball_pass_render_probe.exe --baseline NEW_BASELINE/ball_pass_render_probe.exe --pack PATH_TO_F564_PACK --output NEW_REPORT
python -B tools/verify_ball_draw_resources.py --capture build/ball-native-v1 --rom "F:/Games/SNES/NBA Live 95 (USA).sfc" --output NEW_NATIVE_REPORT
python -B tools/build_gameplay_hud_probe.py --kind cli --output NEW_CLI
python -B tools/capture_ball_pass_sequence.py --exe NEW_CLI/hud_cli.exe --rom "F:/Games/SNES/NBA Live 95 (USA).sfc" --pack PATH_TO_F564_PACK --output NEW_SEQUENCE
```

## Remaining boundaries

Independent head/body mirror flags, source jersey/head ordering, ball interleave and complete graphics queue/OAM allocation remain separate D1 work; the current compositor still has a combined direction API. This repair preserves that existing behavior and does not claim it correct for every facing combination. It corrects the proven body-resource substitution without changing ball coordinates to compensate. The source's body/head resource distinction is intentional and is preserved. No full native graphics/scanout timing, entire pass implementation, human enablement or whole CPU suite/image-golden acceptance is claimed. Root will combine its separately accepted direction patch and run integration checks after independent review. No stale baseline or native golden is refreshed here.
