# Gameplay work remaining after the tip-flow checkpoints

Recounted 2026-08-27 with `DumpTipoffFlow.java`, saved in the bank86 Ghidra
project. Fresh contiguous listing:
`.analysis/tipoff-flow-proof-20260827/ghidra/tip_flow_bank86.txt`.
`ghidra-final-census.log` confirms Save succeeded. Each count below is a
decoded instruction start, not bytes, estimated effort, or a whole-game
completion percentage. None of these bounded rows is in the verified ledger.
Some already have partial C implementations: pending means complete native
translation/adoption **and verification**, not necessarily absent C code.

## Bounded instruction census

| Area | ROM range(s) | Pending instructions | What remains |
|---|---|---:|---|
| Jump/reach startup | 86:EC32-ECF8 | 77 | Replace frame-based initial jump/pose selection with native inputs and cadence |
| Loose-ball/jump continuation | 86:ECF9-EE75 | 162 | Complete downstream reach/jump decisions and animation integration |
| Ball initialization prefix | 86:E054-E0AB | 31 | Native startup state/velocity; this is only a prefix, not the complete toss dispatcher |
| Stationary defensive idle selector | 86:E39A-E3CA | 20 | Natural state7 selection; its animation cadence is already implemented |
| Wider defensive pose caller | 86:E3E1-E4A6 | 79 | Integrate caller decisions before claiming native idle distribution |
| Inbound owner continuation | 86:F43A-F4F1 | 89 | Replace/reverify remaining compatibility caller behavior |
| Inbound target arrival test | 86:F4F2-F51F | 19 | Exact compensated target boundary and continuation |
| Inbound readiness/state writes | 86:F54F-F58E | 23 | Whistle, transfer, facing and arrival bookkeeping |
| CPU inbound timing gate | 86:F58F-F5BA | 16 | Signed controller, airborne, timer and RNG conditions |
| Inbound receiver candidates | 86:F5BB-F60A | 35 | Ordered candidates, fallback and timeout rules |
| Inbound launch/orientation gate | 86:F60B-F653 | 31 | Exact basket-side gate and shared pass handoff |
| Inbound continuation return | 86:F654-F668 | 11 | Timer reset and return/caller composition |
| Timeout confirmation prefix | 86:844E-8467 | 9 | Connect confirmation to the complete timeout flow |
| **CPU/gameplay subtotal for these bounded slices** | | **602** | Not a census of every unfinished routine |
| Optional human inbound steering | 86:F520-F54E | 19 | Outside current CPU-vs-CPU scope |

The older **323 CPU/animation** subtotal is still valid: defensive99 +
CPU inbound224. Including the separate human steering19 makes that neighborhood
342. The newly expanded jump/toss prefixes add270; timeout adds9. Thus602,
or621 including the human branch. Do not add323 again to this table.

## Important work not represented by that subtotal

| Area | Count/status | Boundary |
|---|---|---|
| Other contact alternatives | 35 gate starts not witnessed by this goal's tip fixtures | The focused geometric replay witnesses143/178; older shared contact proofs do not make the new tip witness branch-exhaustive. Not added to602. |
| Complete initial toss/formation/dispatcher | Not fully censused | E054-E0AB above is only31 instructions; the current initial toss is still quadratic and the jump presentation uses frame constants. |
| Ordinary pass adoption | Shared306-instruction launch body already verified; caller count not censused | Tip uses the complete99C4/9C45 implementation; ordinary/inbound adapters still need adoption/comparison. Do not call306 untranslated instructions. |
| Held-ball/dribble/contact pose consumers | Not fully censused | B649/B66A integration still uses some legacy tick-derived resources/phases; isolated animation proof is not whole-call-chain parity. |
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

The default C run first contacts at204 and catches at222; the recorded native
calls are200 and220. That difference remains visible and is not concealed
with a fitted frame offset. The initial toss/jump rows above are the next
focused work, before claiming a ROM-identical tip-off presentation.

Verification ledger:7,585/27,901 captured address positions,27.19%.
Detailed checkpoint evidence and release results: `tipoff-flow-plan.md`.
