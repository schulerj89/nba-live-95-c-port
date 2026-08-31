# Native audio event-consumption candidate

This candidate is a bounded translation of `$82:FD65-$FF84`, not a completed
audio system. It is not yet called by the production game's NMI/presentation
scheduler. The previous audio-only RNG and edge latch remain in the main
implementation until integration replaces them coherently.

Work stopped at the user's request with this candidate on
`work/audio-events-20260830`, based on `e1bc0d4`. The final unchanged replay
`build/audio-events/native-stop-freeze.json` passes, as do all seven verifier
tests and the playback-discard/malformed-input regression. Independent review
of this new module is still pending. This is a WIP handoff, not an accepted
production audio checkpoint; it is deliberately absent from `nba95_sources.txt`.

## What is translated

`nba_audio_events_dispatch` owns the native `$13E7/$13E9` event-consumption
contract and the ordered calls to `$80:9DF3`, `$80:A82F`, and `$80:9F0F`.
It receives a pointer to the actual shared `$07F6` word. Each set sound bit
is cleared before its RNG/callee call. Bits0..9 consume one RNG result each;
bits10..13 select fixed commands. Unknown `$13E7` bits14/15 remain pending.
It does not suppress an event because the same bit was set last dispatch.

Crowd Sound OFF clears `$13E9` at entry. Otherwise all four low crowd bits
are handled separately, with the native queue index and delay30. Bit2 consumes
one shared RNG result and indexes the two original parallel tables. Unknown
high crowd bits remain pending when Crowd Sound is enabled. An OFF setting
therefore changes RNG consumption when it suppresses that randomized crowd
event; preserving a private audio RNG would miss a real gameplay dependency.

Bounce uses the return A from the indexed sound driver, masks its low byte
to select a voice, then applies the original `$13E5` volume calculation.
The sample/voice allocator is a downstream dependency. The parity probe
supplies captured native command-return A values explicitly and separately
from expected calls/output. This is not an implementation of `$80:9DF3`.

The ROM tables at `$82:F822-$F899` are translated control constants. No
recorded audio, rendered screenshot or ARAM capture is added as a production
asset. Host playback is optional: discarding playback operations still
consumes exactly the same event bits and shared RNG.

## Independent native evidence

The canonical ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Both final captures run the unpatched ROM in a private portable Mesen with
empty save folders, fixed zero power-on RAM, one physical controller,
frame skipping disabled and no video filter. The launcher uses an explicit
per-process environment dictionary, never shared global environment writes.
Observed Lua home and pre/post settings/source hashes are retained.

Canonical evidence in the main repository:

- `.analysis/runtime-options-20260830/natural-v3/`: normal title/Main/Options/
  Team/Player Setup input. Both volume bars visit every0..45 value; Music
  Mode visits OFF, MONO, STEREO; Crowd Sound visits OFF/ON and returns ON.
  Player Setup selects center through LEFT. All ten first-court actor
  controller words are`FFFF`; all five selection words are1. No state writes.
- `.analysis/runtime-options-20260830/controlled-v3/`: the same natural boot
  and normal center selection, followed after100court frames by49 explicitly
  logged audio-input injections at the real `$82:FD65` entry. Cases cover
  adjacent identical sound bits without an intervening quiet dispatch, all
  fourteen sound bits, all four crowd bits, Crowd OFF, combined families,
  unknown high bits, zero/high-bit RNG seeds and bounce-volume boundaries.
  Only `$13E7/$13E9/$13E5/$17BB`, plus three explicitly declared `$07F6`
  seeds, are changed. No PC/register/ROM/PPU/SRAM write occurs. Later execution
  is labeled **post-controlled continuation**, never natural execution.

The original source and CPU/event/WRAM/APU/DSP traces are retained. v1 CPU
captures and v2 downstream-audio captures remain evidence; v3 adds the missing
sound-driver return A boundary so the bounce voice is independently known.
These are harness extensions, not output-selected timing retries.

Both final capture scripts have SHA256
`1dc899f48ea6280be12d12bfc6edad953aad2e3cb334474481ec591ba8bf0f19`.
Final manifest hashes are:

| Capture | SHA256 |
|---|---|
| natural-v3 | `79e74086e65e0d20575bac236c2a1d0d79b4a53db163112d732588c7e5379b15` |
| controlled-v3 | `bb8bc524504f72c00640cee990ebdd0dc8e973f85a08dd0de1b090f7c835520e` |

`tests/fixtures/audio-events-native-witnesses.json` is539,147bytes, SHA256
`20762a4a35ed00cf7f04084dacead537cf98884f57262b44244fa1aafc4393b3`.
It retains the projected native inputs, external driver returns, ordered call
arguments, owned state at each call and final owned state. Original manifests
are copied verbatim and bound to raw manifest hashes. Capture/raw file hashes
remain visible. This is a deliberately named projection, not a lossless copy
of every CPU/DSP state or a whole-game hash. The normalizer never executes C.

## Gate results and denominators

| Gate | Population / result | Exclusions |
|---|---|---|
| Native routine projection versus compiled C | PASS2,612 dispatches:2,206 natural capture +406 controlled capture;173 ordered command/volume/queue operations, exact final three words and nine fields per operation | Full native registers, driver internals, every RNG/table combination, natural C boot/input/timing |
| Natural execution |2,200court frames;46nonzero event dispatches;50RNG advances inside audio dispatch out of4,073 observed shared RNG calls | C-versus-ROM whole-frame trajectory |
| Controlled execution |49injected dispatches, including all14sound and4crowd bit branches; later retained high-bit execution stays labeled controlled continuation | Natural occurrence of every edge case |
| Protocol/evidence tests | Seven tests including every represented output and all171operation-field mutations, malformed output, changed provenance/exit/settings/source identities | ROM behavior by themselves |
| C playback-discard regression | All2,612native final event/RNG states unchanged with a NULL playback sink; malformed/missing callee inputs rejected | Audible output/SPC parity |

Native gain observation independently found`min(3*value,127)` at
`$87:8C2D-$8C65`, writing `$15C5/$15C7` and then driver `$0627/$0628` through
`$80:9C47`. All46input values on both bars are observed;186gain calls are
recorded. The current host gain functions are not translations of this chain.
Gain/mode implementation is pending; these observations are not credited to
the event translator.

## Source review and reproduction

Fresh Ghidra output with original instruction bytes:
`.analysis/runtime-options-20260830/runtime_options_bank{80,82,87}.txt`.
Fresh verified-recompiler output and source manifest:
`.analysis/runtime-options-20260830/reference/`; counterpart names
`GameplayAudioEvents_M0X0`, `PresentationAudioService_M0X1`,
`AudioSharedRng_M0X0`, and `WorkingAudioGain_M0X0`.
The generated C is a translation reference, not an independent behavioral oracle.

From the audio-events worktree:

```powershell
.\tools\build_audio_events_probe.ps1
python tools/test_audio_events_verifier.py
python tools/test_audio_events_probe.py --probe build/audio-events/audio_events_probe.exe --fixture tests/fixtures/audio-events-native-witnesses.json
python tools/verify_audio_events.py --fixture tests/fixtures/audio-events-native-witnesses.json --probe build/audio-events/audio_events_probe.exe --report build/audio-events/auditor-new.json
```

Reports must be new files. Candidate result:
`build/audio-events/native-final-v2.json`. New raw captures can be made with
`tools/capture_audio_contract.py --mode natural|controlled --mesen <Mesen.exe>
--output <new directory>`; the script creates its own portable runtime.

## Integration contract and remaining audio work

Native `$80:8576` calls `$82:F89A` from the NMI/presentation service when
`$1419` enables it. Captured stack return`$80:8579` confirms this caller.
There are2,199post-court service entries and982main gameplay loop iterations
in the natural run. Calling the new routine simply after each C actor update
would therefore still be a scheduler approximation. Root owns that boundary.

The future adapter must copy the live event words into `NbaAudioEventState`,
pass`&tipoff->rng.state`, run the translator at the verified native service
boundary, then publish its remaining event words back. It must run with host
audio disabled and during the native service's eligible lifecycle states.
The sink must use a logical resident-driver equivalent that returns native
voice handles; returning a host round-robin voice is not equivalent. The
existing host allocator2..7 differs from native captured returns0800/0801/
0502etc. Bounce masks those values to0/1/2etc before the volume call.

Still pending: logical sound-driver allocation/queue behavior, BRR/sequence
resource ownership and sequencer timing, exact gain/mode consumers, crowd-bed
and reaction scheduling, native audio samples, normal production caller wiring,
and differential NMI/shared-RNG integration. Existing Setup playback still
uses captured command/DSP streams and an ARAM snapshot; that production
provenance/sequence-logic gap is not fixed by this module. No audio or game
completion claim follows from this bounded PASS.
