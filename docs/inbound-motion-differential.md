# CPU inbound motion differential

Verified 2026-08-29 against the original ROM at `$86:F43A-$F4F1`.

## Native evidence

Mesen executed 500 natural calls to `$86:F43A` with no patched program
counter, ROM, flags, or stack. The raw capture includes 111 calls which reach
the non-arrival motion slice, 387 calls already inside the arrival box, and two
calls which continue through pass launch. The durable motion fixture retains
all 111 non-arrival calls, covering three acceleration profiles and both signs
of velocity.

`tools/verify_inbound_motion_vectors.py` replays the retained inputs through
the production helper and compares the exact next-pass X/Y velocities and
direction. It reports zero mismatches across 111 calls, including 104
nonnegative and 118 negative velocity components. The live 63,800-frame CPU
regression separately proves that the same helper drives both teams through
scoring, installation, arrival, pass transfer, and resumed live play.

## Corrections found by the differential

- `$85:963D` commits position before dispatching `$86:F43A`; the port was
  integrating position a second time inside the inbound routine.
- Arrival and side gates read the signed integer words of 24.8 coordinates,
  not presentation-rounded pixels.
- `$85:B3C9` selects damping when distance is at most eight; the prior port
  always accelerated.
- Native negative division by 16 uses the repeated compare/rotate bias, so a
  value such as -128 produces -7 rather than host-language truncation -8.

The reusable `nba_gameplay_inbound_motion_step` helper now owns those rules,
and the live actor path calls it directly. Exact 24.8 actor positions were
added to gameplay telemetry so boundary assertions cannot silently round.

## Scope boundary

Coverage credit in this checkpoint stops at `$86:F4F1`. Arrival readiness,
timer/random gates, receiver selection, pass launch, and the return wrapper at
`$86:F4F2-$F668` remain uncredited until their owned outputs receive the same
durable native-vector treatment. The raw 500-call capture is useful working
evidence but is intentionally not the maintained fixture.
