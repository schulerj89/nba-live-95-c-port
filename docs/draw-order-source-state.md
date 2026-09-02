# Persistent source draw order

`nba_draw_order` owns the twelve source draw pointers and their depth words.
The [dribble animation correction](dribble-animation.md) connects it to
`NbaTipoff`: initial placement performs FBFF, scheduled object-origin latches
perform one FC80 pass, and period restarts perform FBFF on the carried list.
The renderer uses that order for actor overlap and the owner's ball/hand
interleave. The standalone period-render component and its frozen evidence
remain separate; full native OAM allocation and CPU timing are not modeled.

`NbaDrawOrder.order[12]` corresponds to `$7E44..7E5B`; `depth[i]` corresponds
to `(34EB + 100*i) + 68` (hexadecimal addresses). The twelve records are the
ten actors, ball `$3EEB`, and basket `$3FEB`. There is one canonical copy of
each order/depth word. The input has twelve explicit X/Y pairs and camera Y.
It contains no WRAM buffer, implicit pointer interpretation, captured output,
default basket position, or timing inputs.

| API | Exact owned operation |
|---|---|
| `nba_draw_order_initialize` | `$80:FBE9..FBFE`: write identity pointers `$34EB..3FEB` in ascending record order. Preserve all depth words. |
| `nba_draw_order_project` | Twelve `$87:A3B6..A3CE` depth blocks, traversing the carried order backwards. Compute wrapped `(Y-X)`, two arithmetic right shifts, then wrapped camera-Y subtraction. Preserve order. |
| `nba_draw_order_pass` | `$80:FC80..FCA1`: compare adjacent pairs `10/11,9/10,...,0/1` exactly once. Preserve depth words. |
| `nba_draw_order_update` | Convenience composition of those projection/pass operations; the caller must own their scheduling and excluded work. |
| `nba_draw_order_full_sort` | `$80:FBFF..FC7F`: project all twelve depths and execute the original 7/3/1 gap comparisons for initial/period placement. |

The source initializer is called at `$86:DA89`. It must not be rerun each
frame or each period. It accepts arbitrary previous order words because the
original overwrites them. Other calls require a bijection of all twelve
original record pointers; null inputs, duplicates, misalignment, or pointers
outside that set refuse without mutation. Structural validation is a bounded
API precondition; it is not a new original-game repair. Initial depth words
are carried, not fabricated as zero. Projection overwrites every depth before
the scheduled pass consumes it.

FC80 loads X=22 and subtracts two before its first comparison. Its CMP/BPL
decision tests bit15 of the **wrapped** `rightDepth-leftDepth`. Equal keys do
not swap. Host signed comparison, sorting by an unwrapped mathematical value,
or a full sort changes original behavior. A reversed ordinary-key list needs
eleven calls to become sorted; the first call deliberately leaves it partly
unsorted. `$80:FBFF`'s exceptional full sort now updates this same persistent
state at initial/period placement. Its wrapped comparisons match all 96
order/depth words in the four existing native period-placement captures.

The depth calculation preserves negative rounding and `$8000` wrap. It does
not consume Z, fractions, camera X, or apparent screen Y after height removal.
The original `$87:A3D1..A435` screen-X/culling/indicator body is excluded;
this module claims only its stated depth and order effects. A caller must
preserve real input/interleaving ownership around that body. The captured
CPU route has stable XY/camera across the observed projection/pass scope.
No general human indicator, OAM publication, CPU/DP/register state, instruction
duration, NMI ordering or renderer scheduling implementation is claimed.

## Basket provenance and integration boundary

Basket X already belongs to `NbaTipoff.court_presentation.basket_x_3fef`.
Project that actual carried value; do not recompute it from anchors changed
by the period-two prefix. Basket Y has original initialization provenance
`$86:DBC2 STZ $3FF3` (bytes `9C F3 3F`). The new native capture brackets that
instruction. Its value is zero both before and after, so that fixture alone
does not distinguish an omitted write; the original opcode establishes the
write. The draw-order API intentionally does not own or initialize basket XY.
The runtime adapter supplies this initialized zero Y, the carried basket X,
actual actor/ball integer XY, and current camera Y. Ordinary updates occur
with the existing retained player/ball-origin latch. The standalone API
continues to leave input and scheduling ownership to its caller.

## Evidence and reproduction

Actual ROM SHA256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
`verify_draw_order.py` checks exact original initializer, projection, pass,
caller and basket-store bytes. The test-only `draw_order_rom_reference.py`
executes a bounded subset of actual opcodes for these isolated source blocks;
it is never linked to runtime and does not interpret excluded children.

The new `.analysis/native-draw-order-v1` capture uses private portable Mesen
settings/home/saves, normal cold boot and menu/controller input only, with
CPU selection1. There are no WRAM, ROM, register, PC or state seeds. All
attempt/config/source/output identities are retained. The run exits0 at frame
4659 and records66 full-WRAM boundaries: four initializer caller/entry/terminal/
return observations, two basket-store boundaries, and five boundaries for each
of twelve actual scheduled draw passes at court240..269. These observed
intervals are not converted into an invented periodic scheduler.

Native verification passes37 isolated component cases /888 order/depth words.
A second check starts from the first observed projection prestate and carries
**C's own** order/depth across all twelve passes, accepting only changing native
XY/camera as subsequent inputs:288 more word comparisons pass. This is a
bounded twelve-pass state differential, not a normal initialization-to-game
prediction. The unmodeled time between initialization and that first prestate
includes other original gameplay and exceptional sorts; no replayed afterstate
is inserted into the runtime API.

Fresh private `/W4 /WX` two-source build-v2 passes. No shared object files are
used. Controlled source tests pass5,668 cases /136,032 typed words, including
1,019,589 original instruction decisions,66,370 source write positions and36
visited PCs. They cover all adjacent slots, full-word edge pairs, projection
and camera edges, varied permutations, ties, and arbitrary carried depth.
Twenty-four initialization/refusal cases, seven null/preservation checks per
probe, eleven carried sorting passes and six malformed binary inputs pass.
Forty-six reachable in-memory protocol mutations reject, including process
types/stderr, duplicate JSON, C word widths/order, native M/X/D/DP/DBR/stack/
chronology, missing artifact/source identities and changed persisted settings.

The native gate rehashes exact source/artifact sets, requires its exact normal
route and isolation settings, validates duplicate-free typed rows and callback
PCs, and checks captured binary16/DP0/DBR7E conditions. It compares gameplay
effects, not general CPU endpoint equality. Every probe receives typed before
inputs only. The original captures remain immutable. Earlier private build-v1
and native-v1 reports are retained; final reports are build-v2/native-v2/source-v1/
protocol-v1. No failed native attempt occurred for this capture.

From this worktree, choose fresh output directories:

```powershell
./tools/build_draw_order_probe.ps1 -OutputDirectory .analysis/draw-fresh
python tools/verify_draw_order.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/draw-fresh/draw_order_probe.exe --native .analysis/native-draw-order-v1 --output .analysis/draw-fresh-native
python tools/test_draw_order.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/draw-fresh/draw_order_probe.exe --output .analysis/draw-fresh-source
python tools/test_draw_order_protocol.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/draw-fresh/draw_order_probe.exe --native .analysis/native-draw-order-v1 --output .analysis/draw-fresh-protocol
```

`capture_draw_order.py --output NEW_DIRECTORY` reproduces native observations
only after coordinating the shared emulator slot; it never changes user Mesen
settings or starts a visible helper window. This document does not authorize
production wiring or claim a fixed ordinary-game frame cadence.
