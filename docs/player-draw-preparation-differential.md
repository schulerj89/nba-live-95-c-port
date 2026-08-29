# Player draw-preparation differential

Verified 2026-08-29 against the original ROM's ordinary CPU-player path in
`$87:A47A-$A6A8`.

## Evidence

Mesen drove the original ROM into CPU-versus-CPU play and retained 2,000
calls immediately before `$87:A6A4` submits to `$80:AD92`. The calls cover
all ten actors, 175 upper/lower resource pairs, all eight directions, both
native priority bands, five control modes, 65 target-facing calculations,
and 30 calls where presentation direction differs from movement direction.

The durable fixtures replay two portable outputs through production C:

- `draw-direction-witnesses.json` compares `$87:A52C-$A5FA`'s target-facing
  selector. All 2,000 calls match with zero differences.
- `draw-preparation-witnesses.json` additionally compares selected direction,
  actor mirror/status bits, upper/lower resources, head resource, OBJ
  attribute/priority and submitted X/Y. All 2,000 calls match with zero
  differences.

The full native routine was also captured at entry `$87:A47A` and return
`$87:A845` for 200 natural calls with no orphan exits. That larger working
capture is used to audit side effects but is not checked into the repository.
The 63,800-frame runtime regression requires a naturally changed mode-10 or
mode-15 draw direction and validates every selected packed player layer.

## Port correction

The renderer previously used actor movement direction for every layer. Native
mode 10 faces the live ball, and mode 15 faces raw receiver `$0946`; the
selector may adopt the target direction or ease by two directions while
leaving actor `+$52` unchanged. The port now uses
`nba_gameplay_prepare_player_draw` for presentation only. Physics and ball
attachment retain the movement-facing resources, and telemetry exposes both
sets so future tests cannot conflate them.

## Credited scope and exclusions

Ledger credit is limited to 128 captured address positions in directly
replayed ordinary-player slices. The unobserved mode-8 body, the mode-14
pointer test, the bug-compatible state-20/21 stale-DP input, resource-to-
number intermediate lookup, selected-human effect path, basket/indicator
queues and SNES DMA remain outside this checkpoint. Final number/head/body
composition retains its separate `$80:AD92-$AEC1` proof.
