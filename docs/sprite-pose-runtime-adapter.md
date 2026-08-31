# Literal player-pose runtime adapter

This checkpoint activates the accepted `$80:AD92/$80:AF1E` player-layer
compositor when the optional `NBPDRAW1` asset is present. It is based on
`744809a9d2ad548f83dedd9dffabce09e3cbda11` and does not edit ball position,
velocity, ownership, attachment physics, the graphics queue, NMI, `NbaGame`,
session/release code, the production source manifest, or the shared asset
pack. Packs without resource 287 retain the prior renderer byte-for-byte.

## Source-owned live inputs

`cpu_advance_actor_animation` and every other reached resource publication
now perform `$87:AB48-$AC22`: clear actor `+$28` bit 15, then set it only when
published display direction `+$52` is below three. The old code did this only
for mode 17. `latch_player_screen_origins` performs the separate
`$87:A5FB-$A609` bit-2 update only after projection/culling admits an actor.
Neither producer changes the other mirror bits.

The live adapter passes raw actor `+$2A/+$2C` as D6/D4, movement `+$4E` as C0,
display `+$52` as C2, the cull-owned status as 47, and the actual caller
attribute/origin. Resource 287 supplies the signed `AC:B6B3[D6]` head order
and direction-indexed `87:A98E[C2]` number resource. Existing `NBPANIM1`
supplies signed `AC:C7E3[D6]` suppression and all attachment tables. Existing
player appearance setup supplies head base and palette offset. Selected head
facing affects DA and bit 2 only; it never rotates D4/D6 or ball coordinates.

The direct framebuffer reverses the native B348 submission stream so lower
OAM indexes win. Upper, lower, and head flips remain independent, and sign of
51 controls head first/last. C0 controls torso/number order. D8 `$0591` keeps
its independent number mirror. `$0884` is graphics-queue work without a pixel
effect; the pixel adapter does not export or claim its live value.

## NBPDRAW1 contract

Resource 287 is 2,144 bytes: a 32-byte `NBPDRAW1` v1 header, all 2,096 bytes
of `AC:B6B3`, and all 16 bytes of `87:A98E`. The builder accepts only the
canonical unheadered ROM SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The upgrade tool accepts only the reviewed 264-resource/version-31 pack,
appends 287, and records every old payload identity. The private tested pack
is 89,444,904 bytes, SHA256
`acc4a436c990fd3a7beab9dadab47d40690ccedf912f7db90e283264ed0f299a`.
No generated pack is committed.

## Verification

- The accepted compositor replay passes 43 native draw groups (387 native
  geometry words and 301 source producer words), all eight directions,
  149,632 controlled opcode cases / 5,386,752 output words, 9,976 exact legacy
  compatibility calls, 10 domain refusals, 28 atomic guards, and 16 malformed
  protocol/build views.
- Fresh 40-translation-unit `/W4 /WX` builds pass for the CLI and runtime
  probe. The input probe independently derives all 16,768 D6/C2 table cases
  from the ROM, compares 696 identity cases with accepted appearance setup,
  checks three atomic refusals, and rejects a corrupted resource 287.
- A separately archived/built 744809a CLI, the new CLI with the old pack, and
  the new CLI with the private pack produce the same 390-row gameplay trace,
  SHA256 `44887d8f1a6196608a617d2e351adcf8220ab28057b4ba0ce968fe2a898cad7b`.
  This includes all exported ball world/screen coordinates, velocities,
  owner, state, actor state, clocks, and RNG. Base and fallback BMPs are
  byte-identical. The activated literal compositor changes 52 RGB bytes in
  the inspected frame, proving the new path runs without a state change.
- The source ownership guard finds five body-mirror publication calls, the
  cull-owned head-bit update, all four live adapter bindings, and zero changed
  ball-state lines relative to 744809a.
- The broad 63,800-row gameplay run reaches and passes its state analysis
  (72 shot selectors, 42 made-run updates, one natural special and the pass
  interruption/recovery). Its first failure is the expected visual-review
  gate at frame 600: the literal compositor RGB SHA256 is
  `cc97b250c177afc4bd238dc6134239b208fc1acc428dddf3db45885a26ec498b`.
  No frame anchor is migrated in this component.

This checkpoint does not claim native B348 child allocation, OAM cursor,
graphics-ring history, NMI budget, scanout timing, AF1E ball interleave,
human controls, inbound presentation, court-logo correctness, jersey-value
correctness, or full-game completion. Those remain separate production gates.
