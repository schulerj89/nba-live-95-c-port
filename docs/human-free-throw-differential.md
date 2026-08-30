# Human free-throw aim differential

Implemented and replayed 2026-08-29. This checkpoint adopts the ROM's
controller-owned two-axis free-throw aim sequence. It is exact for the seven
retained state-transition witnesses described below; it does **not** make the
port's wider human gameplay path playable or equivalent.

## Native boundaries

| boundary | evidence status |
|---|---|
| `$87:9CBF-$A017` | Mapped composite dispatcher. The typed helper adopts the represented state/aim decisions in the retained assignment-zero sequence, but artwork, setup PPU/resource calls, alternate controllers, and complete common-launch effects and same-call ordering are excluded. The broad ledger row therefore receives no address credit. |
| `$87:A018-$A045` | Exact, credit-eligible aim oscillator: add `$0986` to accumulator `$0984`, subtract 110 for every cursor step, increment `$0980`, and wrap the cursor at 112. |

The initializer clears `$0980/$0984` and derives the cursor quantum from the
shooter's free-throw rating as
`$0986 = $0226 - 2 * (rating - $80)`, with native 16-bit arithmetic. The human
branch requires a non-negative actor controller assignment and a nonzero team
context `+$3B`. The shoot-button test is the ROM mask `$C0C0` (B or Y).

## Genuine Mesen input capture

The canonical source capture is
`.analysis/human-free-throw-native-20260829-v4`. It emitted 1,556 native
entry/exit vectors with zero orphan exits and zero shared-exit callbacks. Its
first B press is delayed for 60 input polls, which exposes a distinct cursor
wrap before the retained state-transition sequence.
`tools/mesen_human_free_throw_control.lua` waits for the real on-court
`$87:A47A` draw path, writes only the documented shooter, ownership,
controller, human-context, free-throw state/count, whistle, and resolution
words, then drives B through Mesen's controller API with `emu.setInput`.

Every retained call enters the native dispatcher at `$87:9CBF` or oscillator
at `$87:A018` and returns through `$87:A017/$A045`. ROM bytes, PC, stack,
processor flags, and RNG are never patched. This is a controlled genuine-entry
capture with real controller input, not a natural foul-to-stripe frequency
claim. The fixture locks ROM SHA-256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`;
the v4 vector corpus SHA-256 is
`a1c252ab961d6e72d4159553706a16176dd151ba4f26e5343d39e4808486dabd`.

## Seven durable witnesses

`tests/fixtures/human-free-throw-aim-witnesses.json` retains one native call
for each input edge. Cursor and accumulator values below are decimal native
words.

| transition | controller condition | exact retained effect |
|---|---|---|
| `3 -> 3` | B/Y clear | First-axis oscillator advances X `4 -> 9` and accumulator `86 -> 62`; state waits. |
| `3 -> 4` | First B/Y press | Oscillator advances X `31 -> 36` and accumulator `50 -> 26`, then locks Y to `36` and clears X to `0`. |
| `4 -> 4` | First press still held | X/Y/accumulator remain `0/36/26`; state 4 does not oscillate while held. |
| `4 -> 5` | First-button release | State changes to 5 and immediately falls through into the state-five body in the same native call: X `0 -> 5`, accumulator `26 -> 2`. |
| `5 -> 5` | B/Y clear | Second-axis oscillator advances X `5 -> 9` and accumulator `2 -> 88`; state waits. |
| `5 -> 9` | Second B/Y press | Oscillator advances X `14 -> 19`, accumulator `64 -> 40`, then joins common launch state 9. |
| `3 -> 3` cursor wrap | B/Y clear | First-axis oscillator advances X `109 -> 2` across the 112-word wrap and accumulator `108 -> 84`; state waits. |

The release-frame fallthrough is a material ordering rule. Treating release as
a return and waiting until the next actor call shifts the second axis by one
oscillator step; the retained `4 -> 5` witness rejects that error.

## Triangulation and C integration

`tools/ghidra/DumpHumanFreeThrow.java` and
`tools/ghidra/Run-HumanFreeThrow.ps1` produce a focused Bank `$87` listing
with labels at `$9CBF/$9D25/$9E39/$9E88/$9EBA/$9F11/$A018`. The generated
recomp reference independently resolves `HumanFreeThrowScene_M0X0` and
`HumanAimOscillator_M0X0`; it corroborates the `$C0C0` button mask, held-button
wait, release-frame fallthrough, state-5 oscillator ordering, and common state-9
launch. Generated recomp C is a readable cross-check only: it is not linked
into the port and is not the behavioral oracle.

The typed implementation is in `src/nba_gameplay_free_throw.c` and its scene
adapter is in `src/nba_tipoff.c`. The adapter initializes the rating
quantum, carries `$0978/$0980-$0986`, reads the host B/Y held mask for actor
controller assignment zero, handles the native controller-loss fallback, and
enters the port's shared state-9 launch scaffold. The aim fields are also
published in gameplay telemetry and the strict-differential projection.

The ordinary host session does not reach that adapter yet:
`nba_tipoff_init` currently forces `cpu_vs_cpu=true`, all actor controller
assignments to -1, and never publishes the native mode-11 `+$3B` human
context. This checkpoint does not invent that missing ownership pipeline. The
scene-level self-test injects those documented inputs to protect adapter
ordering, including selection of human aim before the CPU-only 120-tick gate.

`tools/normalize_human_free_throw_vectors.py` reduces the native corpus to the
seven checked witnesses. `tools/human_free_throw_vector_probe.c` calls the typed
C helper, and `tools/verify_human_free_throw_vectors.py` compares every result,
state, cursor, accumulator, and step word. The release build runs this verifier
from `build.ps1`; the checkpoint result is seven cases, zero mismatches. The
Tipoff self-test separately exercises the scene `3 -> 4 -> 5 -> 9` adapter
contract.

## Explicit exclusions

- Aim artwork and presentation words `$0988-$098E`, including exact cursor
  sprite/OAM/DMA behavior, are not implemented by this slice.
- Global player assignment and the complete menu-to-game human ownership path
  remain incomplete. The ordinary runtime free-throw adapter remains dormant;
  it consumes the single host pad for controller assignment zero once that
  context is supplied; it is not proof of all pads, player switching, or
  multiplayer behavior.
- Common launch `$87:9F11` also clears actor `+$4A`, cancels both animation
  channels, installs action 22 and falls into the state-9 body. The current
  fixture checks only represented state/aim words, so complete common-launch
  effects and same-call ordering remain excluded. The composite dispatcher and
  common-launch code receive no address credit until they have a wider native
  oracle.
- Human movement, offense, defense, dribbling, passing, ordinary shooting, and
  their complete animation/camera/audio consequences remain outside this
  checkpoint.
- The native capture seeds a documented stripe attempt at a real gameplay draw
  boundary. It does not prove that every foul, bonus, substitution, and shooter
  selection caller naturally reaches this path.
- Whole-game initialization, scheduler timing, and strict trajectory parity
  remain failing separately. See `docs/parity-gap-report.md` and
  `docs/differential-testing.md`.

CPU aim, attempt cadence, make/miss resolution, and rebound/inbound behavior
retain their independent evidence in
`docs/free-throw-completion-differential.md`.
