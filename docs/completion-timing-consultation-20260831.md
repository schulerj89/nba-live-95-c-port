# Max consultation: next timing implementation boundary

Received 2026-08-31 from the existing read-only task
**Diagnose native NBA95 Rules scheduler…**,
`01a05634-5316-78c0-bb36-f9cdfd3b562e`, for integration baseline `f318478`.
This records planning advice from current source/audits. No new tests, native
captures or implementation were performed by the consultant. Root and scheduler
must implement the contract and obtain independent verification; advice is not
an acceptance result.

## First unit and persistent owner

Compose original cold reset, IPL/upload and resident handoff with one persistent
CPU/SPC clock-and-bus owner. Store it alongside `NbaGame.session`, outside
`NbaGame.scene`: the scene union is cleared on transition (`nba_game.c:305` at
this baseline). Current initialization enters the license screen without the
original reset/upload path. The owner must survive menus, gameplay, pause,
track changes and host-audio muting.

It owns the master timeline and SPC phase, directional/staged port latches,
timer history, IPL/ARAM/control/DSP state and active source continuations.
Shared game fields keep one canonical owner; scene/HUD/audio adapters must not
create independent copies.

Implement the original reset caller and `$80:AB06` uploader against the bounded
IPL path, deriving the1,264-byte upload and entry registers from ROM transactions.
The first acceptance boundary is the real completion of SPC `$0384`'s F1 write
and continuation at `$0387`, followed by the existing clear to the explicit
`$03DB` `OR A,$F3` DSP-read boundary. This is normal-initialization progress,
not a completed audio acknowledgement or full DSP implementation.

Advance each actual bus cycle before committing its effect. Reuse the accepted
F1 commit at its documented post-cycle boundary, preserving visible/staged
inputs separately from outputs and timer-edge history. The existing visible-input
helper is not a CPU-port-write adapter. Pending-read APIs deliberately refuse
completion: do not clear their stop flags or resume with captured state. Hardware
responses and compiled source continuations must close those boundaries. Derive
power-on/reset and clock-ratio/phase rules from source; unresolved timer/DSP
effects remain explicit stops.

## Dependencies after that unit

1. Finish initialization after03DB, timer progression/read-clear and048B's
   nonzero service path, required DSP effects and complete command bodies.
   Preserve acknowledgement-before-body ordering.
2. Complete CPU channel-off handshakes, sample/resource upload and sequence
   startup, then A2CE sequencing and the remaining A137 command/return paths.
   The two longer backdrop routes still stop at A2CE. Close event allocation,
   real return-A and shared RNG; advance through actual menu dwell.
3. Connect accepted codec/backdrop/header work to CPU bus, refresh, DMA,
   controller reads and NMI through RTI, including the true predecessor work.
   Queue/epoch leaves express intent, not elapsed time. A synchronization point
   must not be assumed to erase phase.
4. Continue the same timeline/publication queue into gameplay and HUD. Model
   source accesses through CC10, D1FD and BBE9; establish ED0D/EDAC callback
   installation/removal, positive-only08DE decrement, expiry, pause/return and
   shared RNG. Period/draw-order state proofs do not supply instruction timing.
   Statistics/ad/foul content remains separate required implementation.

## Three acceptance gates

**Normal bootstrap:** from the declared hardware power-on profile and canonical
ROM only, predict uploader transactions/bytes, SPC entry and post-F1 `$0387`
state/clock; continue to03DB without preloaded ARAM/register snapshots or a
fabricated DSP response. Treat unchanged native observations only as expected
outputs; retain exact source/build provenance.

**Rules phase:** continuous initialization and unchanged inputs produce loaded
epochs72/15/71/15 and after-wait73/16/72/16, then pass all retained state/RGB/VRAM
checks. Add a second natural dwell duration with its own independently captured
expectations, without visit flags or timing tables. The158 current mismatches
remain failures until this passes.

**Shared HUD timing:** carry the same owner into ordinary gameplay; pass strict
CC10/D1FD/BBE9 checks without the two08F6 exemptions, plus native pause08DE and
queued upload order. The39945-to39944 crossing must arise between the real reads,
not from a one-tick correction. Direct-scene images alone do not satisfy this.

The first unit excludes full DSP/audio fidelity, all sound commands, HUD content,
human enablement and period repair; these remain in the overall plan. No general
production opcode interpreter, captured return/port/phase injection, empirical
cost, golden replacement or widened exemption is permitted. Preserve original
quirks with comments, including08FF omission and F1/poll ordering.

The consultant reports high confidence in ownership/dependency boundaries.
The minimum DSP/timer subset for a complete first acknowledgement and the total
gameplay timing closure remain unproved. Do not estimate them as simple wiring
or invent cycle totals.
