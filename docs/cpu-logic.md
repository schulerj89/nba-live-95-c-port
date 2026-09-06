# CPU logic workflow

The native gameplay scheduler separates actor physics from behavior decisions.
`$87:8EFB-$8F92` visits all ten actors with physical delta `$C6=2`.
`$87:8F13-$8F5E` eases display direction before `$87:AAB2` advances
animation; `$85:963D-$985F` then resolves locomotion and commits the velocity
selected on the previous actor pass. Global ball, contact, and role work
follows. The later `$87:9244 -> $87:9BD3/$9BD0` mode jump table dispatches the
actor's current `+$5E` behavior. Production now preserves this complete phase
order for mode seven: `cpu_update_all_actors` eases and advances its existing
animation before committing physics, while
`cpu_update_actor_behaviors` runs `$86:994C` after the globals. Direct
native-vector adapters intentionally call only the bounded behavior being
replayed.

The dispatch table currently maps modes 0 through 17 to no-op, ordinary CPU
offense/defense continuations, dead-ball and action holds, knockdown, target
override, receiver, shooter, layup, and pass executors. Each exact routine is
ported and credited separately. A table target in production does not by
itself mean the full target routine has native parity evidence.

## Mode seven dead-ball hold

`$86:994C-$99C3` consumes these inputs:

- full-word live state `$0936` and offense group `$093A`;
- current actor pointer `$96`, actor slot `$C2`, and physical delta `$C6=2`;
- integer position `+$04/+$08/+$0C`, planar velocity `+$0E/+$10`, signed
  controller assignment `+$16`, timer `+$60`, direction latch `+$66`, movement
  boost `+$72`, and current/requested direction `+$4E/+$50`;
- actor team/roster identity and global owner `$093E` through the existing
  movement-profile and velocity child.

The routine takes the following paths:

1. Any full-word live state other than `$0082` requests animation state 3 and
   calls `$86:9846-$986C` to restore mode 1 or 2, behavior timer `$2F`, and
   cleared timer/flags/status.
2. In state `$0082`, a nonzero integer Z word returns immediately. Fractional
   Z does not hold the timer.
3. A grounded actor subtracts two from `+$60`. A negative 16-bit result takes
   the same state-3 and restore path.
4. A CPU actor calls `$85:B3AA -> $85:A82C` with world target `(0,0)`. This can
   accelerate toward origin or damp existing velocity to zero. Human actors
   preserve velocity and boost.
5. Zero post-child planar velocity requests state `$10`, copies `+$50` to
   `+$4E`, and clears `+$66`. Moving actors request state `$0E`; any nonzero
   prior `+$66` flips `+$4E` by four and copies it to `+$50`, then the routine
   writes `+$66=1`.

Animation requests use the ROM-backed channel command equivalent of
`$87:B3BD`. Resource resolution remains in the later animation/render stage,
so this routine preserves `+$2A/+$2C` resources and does not publish the body
mirror bit early. The command tables and per-player movement profile come from
the user-supplied asset pack (`build/nba95_assets.pak` by default); no asset
bytes are stored in the witness fixture.
Parity is limited to a valid pack with the selected team/roster movement
profile and the canonical actor-pass delta `$C6=2`; the host fallback profile
used after a failed lookup is not a native-parity claim.

`tests/fixtures/cpu-mode-seven-witnesses.json` contains 19 compact calls from
two byte-identical genuine-entry Mesen captures. Each call enters at `$86:994C`
and exits at `$86:9961` or `$86:99C3`; no PC, stack, ROM, RNG, or child result
was patched. `tools/verify_cpu_mode_seven_vectors.py` and
`tools/cpu_mode_seven_vector_probe.c` compare all 50 represented fields with
an exact output shape. They cover full-word state comparison, positive and
negative integer Z, grounded fractional Z, timer `2/1/$8002/$8001`, X-only and
Y-only movement, zero versus arbitrary nonzero direction latch, human boost
preservation, CPU damping and steering, post-damping stationary selection,
animation boost remapping, matching state, and a negative animation lock. A
separate full-scheduler check proves the old velocity moves the actor, a newly
installed stationary `$10` animation is not cadence-advanced in the same pass,
display-direction easing selects the pose resource before physics, and
live-state/ownership changes injected at the real post-physics observer
boundary are visible to the later mode-seven dispatch.

The changed whole-game call order exposed `$FFFF` at retained C trace frames
58472-58473. This is valid `$80:CEE7` state, not missing telemetry: the trace
holds `$789E`, advances to `$F13C`, holds, advances to `$FFFF`, holds for two
frames, then reaches `$D975` through `$E279` in two calls. The focused probe
checks `$F13C->$FFFF->$E279` and zero recovery to `$9146`; this adds no RNG
routine credit.

The host narrows controller assignment and facing to their established runtime
types. The retained native calls cover the real `$0000` human and `$FFFF` CPU
controller words and directions 0 through 7; arbitrary noncanonical high-byte
controller or direction words remain outside the claim. The mode-seven phase
correction is deliberately scoped; other behavior modes retain the existing
partial whole-game scheduler model. Whole-game launch state, scheduler
history, and RNG history also remain outside this bounded routine result.
Mode eight `$86:C6AD-$C758` has production behavior and existing C tests but
still lacks its own coverage-crediting native differential.

The next planned bounded dispatch target is `$86:F0B7-$F0FC`, control mode
nine's timed target override. Its recovery-inhibit, signed timer, steering,
final-window damping, saved-mode restore, and animation-command branches need
their own genuine-entry capture and strict production replay.
