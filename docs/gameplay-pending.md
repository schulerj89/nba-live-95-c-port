# Gameplay work remaining after the tip-flow checkpoints

Recounted 2026-08-28 with `DumpTipoffFlow.java`. Fresh contiguous listing:
`.analysis/pending-diff-ghidra-20260828/tip_flow_bank86.txt`.
The old E054 start was inside a JMP operand; the real ball initializer begins
at E056 and has30 instructions, not31. That prefix is now verified/adopted.
See `pending-gameplay-differential-plan.md`. Each count below is a
decoded instruction start, not bytes, estimated effort, or a whole-game
completion percentage. The remaining nonzero rows are not in the verified ledger.
Some already have partial C implementations: pending means complete native
translation/adoption **and verification**, not necessarily absent C code.

## Bounded instruction census

| Area | ROM range(s) | Pending instructions | What remains |
|---|---|---:|---|
| Jump/reach startup | 86:EC32-ECF8 | 0 (77 verified) | Parent decision, caller cadence, actor motion and animation are runtime-wired |
| Loose-ball/jump continuation | 86:ECF9-EE75 | 0 (162 verified) | Parent continuation wired; far EAA8 child remains separate/outside this range |
| Ball initialization prefix | 86:E056-E0AB | 0 (30 verified) | Complete bounded prefix; later toss dispatcher remains separate |
| Stationary defensive idle selector | 86:E39A-E3CA | 0 (20 verified) | Native selector replay and runtime binding complete |
| Wider defensive pose caller | 86:E3E1-E4A6 | 0 (79 verified) | All observed exits replayed; target-pass adapter is runtime-wired |
| Inbound owner continuation | 86:F43A-F4F1 | 89 | Replace/reverify remaining compatibility caller behavior |
| Inbound target arrival test | 86:F4F2-F51F | 19 | Exact compensated target boundary and continuation |
| Inbound readiness/state writes | 86:F54F-F58E | 23 | Whistle, transfer, facing and arrival bookkeeping |
| CPU inbound timing gate | 86:F58F-F5BA | 16 | Signed controller, airborne, timer and RNG conditions |
| Inbound receiver candidates | 86:F5BB-F60A | 35 | Ordered candidates, fallback and timeout rules |
| Inbound launch/orientation gate | 86:F60B-F653 | 31 | Exact basket-side gate and shared pass handoff |
| Inbound continuation return | 86:F654-F668 | 11 | Timer reset and return/caller composition |
| Timeout confirmation prefix | 86:844E-8467 | 9 | Connect confirmation to the complete timeout flow |
| **CPU/gameplay subtotal for these bounded slices** | | **233** | Not a census of every unfinished routine |
| Optional human inbound steering | 86:F520-F54E | 19 | Outside current CPU-vs-CPU scope |

The remaining **224 CPU/animation** subtotal is the CPU inbound continuation.
Including the separate human steering19 makes that neighborhood243. Timeout
adds9, giving233 without human steering or252 including it.
The corrected starting core was601; ball initialization30 and jump/reach239
are now verified. Do not add323 again to this table.

## Important work not represented by that subtotal

Jump/reach checkpoint details: `jump-reach-differential.md`. All239 parent
starts have native decision witnesses and production integration. The far
EAA8 child and graphics payload DMA are separate boundaries, not silently
credited to the parent.

| Area | Count/status | Boundary |
|---|---|---|
| Other contact alternatives | 35 gate starts not witnessed by the tip-flow fixtures | The focused geometric replay witnesses143/178; older shared contact proofs do not make the new tip witness branch-exhaustive. Not added to the bounded subtotal. |
| Complete initial formation/dispatcher | Not fully censused | Ball initialization, countdown, free-ball physics and EC32 jump are wired; wider formation/presentation dispatch remains uncensused. |
| Ordinary pass adoption | Shared306-instruction launch body already verified; caller count not censused | Tip uses the complete99C4/9C45 implementation; ordinary/inbound adapters still need adoption/comparison. Do not call306 untranslated instructions. |
| Held-ball/dribble/contact pose consumers | Appearance-aware fallback fixed; instruction parity incomplete | Legacy-tick callers now apply actor +A8/+6C and all derived resources exist in the asset pack. B649/B66A exact attachments retain strict replay proof, but 294 starts across the wider appearance/setup/draw census still lack observed execution or instruction-level replay. |
| Final player sprite placement | Native projection/culling behavior implemented; rare presentation branches remain | `$87:A3BB-$A43B` now owns exact integer-word projection and visibility. Five statically translated high-jump culling starts lack a live witness; 23 other unobserved starts belong to queue/effect/ball setup. The human off-screen indicator remains separate; see `gameplay-sprite-jitter.md`. |
| Camera input ownership and framing | Camera core/callers already verified; wider input producers not fully censused | Remaining ball/actor trajectories, alternate flags08BC/08CC, and owner093E inputs can still produce poor framing. Do not re-add the completed99 camera instructions. |
| Court presentation | Not fully censused | Animated crowd CHR and downstream BG1/backboard/window composition; pack-derived map geometry is already checked. |
| Timeout/period/substitution orchestration | Not fully censused | Initial43200 clock seed, period transitions, grants and bench promotion beyond the nine-instruction timeout prefix. |
| Tip event/pointer provenance | Not fully censused | Slot11 event descriptor is preserved; downstream consumer and DP E0/E2 provenance are not invented. |
| Debug scheduler evidence | Integration audit | Some scheduler telemetry is derived from cadence rather than recording each call; contact/possession event timestamps now come from actual execution. |
| Wider scoring/fouls/AI behavior | Not fully censused | Native-call correctness and sustained simulation do not establish complete-game trajectories, frequencies or all rule branches. |

## What this goal completed

Contact -> temporary catch -> RNG-selected receiver -> native launch/free-ball
physics -> second physical contact -> actual ownership. The forced frame220
winner, strategy/RNG reset and ownership award are gone. First contact records
the ball source10 in0942, as the native D3C6/B04C caller does.

Native proofs:340 geometry calls,5 receiver calls,126 launch calls,
164 completion calls,22 acquisition-core calls and17 wrapper calls, all zero
represented-output mismatches. These datasets include deliberate controlled
WRAM cases and duplicate natural witnesses; they are not independent whole
matches. C ABI locals replace SNES stack preservation; cycle parity is excluded.

After the current configured C launch, first contact is164 and possession is186.
The retained native configured capture contacts at185 and possesses at206; its
actor scheduler begins roughly20 frames later. That context offset remains
visible and is not concealed with a fitted frame constant. Full matching-start
trajectory equivalence still requires configuration/state alignment.

Verification ledger:7,826/27,901 captured address positions,28.05%.
Detailed checkpoint evidence and release results: `tipoff-flow-plan.md`.
