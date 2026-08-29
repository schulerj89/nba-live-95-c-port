# Dead-ball and inbound closure differential

## Native boundaries

- `$87:9B38-$9BC8` resets a five-second/dead-ball sequence.
- `$86:F520-$F54E` maps a human inbounder's controller direction through the
  16-byte table at `$86:F669` when actor `+$16` is human and `+$72` is active.
- `$86:F5D2-$F60A` tries the second/third CPU receiver selectors and reaches
  the timeout fallback when none is valid.

## Evidence and production binding

The dead-ball fixture retains two controlled real-entry ROM calls with no
PC, ROM, or stack patch. Its probe compares 36 outputs and proves the native
live-state, timer, clock, owner, ball, rim, selector, coordinate, and phase
updates. The production reset now consumes the actual possession actor
(`$093E`) as the previous actor instead of the provisional inbound slot.

The alternate-selector fixture retains a valid second-selector call and an
all-invalid timeout fallback. Both replay through the same production helper
used by live CPU inbound flow. The human branch exhaustively checks all 16
SNES direction nibbles plus the CPU and inactive-human preservation gates;
the live tip-off update supplies controller 0's current held-button word.

## Regression gate

`build.ps1 -Test` rebuilds all three probes and requires zero mismatches before
the longer CPU gameplay smoke/regression suite runs. These tests complement,
rather than replace, the existing 387 natural arrival calls, natural launch
witnesses, and 63,800-frame CPU endurance test.
