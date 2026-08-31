# Dynamic dribble ball attachment

## Finding

The attachment tables and packed resources were already correct. The missing
parity was the resource source used by the production wrapper.

`$87:B649-$B669` and `$87:B66A-$B67B` call the pose-point composers at
`$87:B832-$B952` / `$87:B953-$B995` with the actor's current `+$2A/+$2C`
resources. The focused Ghidra listings, native wrapper fixtures and generated
recomp bodies agree on that boundary. They do not rebuild an ordinary dribble
frame from an independent host tick.

The same wrappers forward literal actor `+$28`. Its sign toggles masks 1/2,
then the low masks independently mirror the lower and upper attachment
inputs. Reconstructing only the sign bit from direction discards pose state
that the live sprite compositor still consumes and can put the ball at a
different point than the submitted hand.

That distinction matters at `$86:E545-$E592`. A bases-9/11 reversal preserves
the current resources while it reverses the channel phase; `$87:AD5B-$AEC2`
publishes the next resources on the next animation cadence pass. C previously
called `actor_animation_resources` from `ball_position_at_actor`, which could
reconstruct a different resource pair from `upper_animation_tick` during this
interval.

## Production correction

`actor_attachment_resources` consumes cached, valid `+$2A/+$2C` whenever the
attachment uses the actor's current direction, and the attachment/collision
adapters now forward literal `+$28`. Corrupt/uninitialized resource state
retains the existing asset-pack resolver fallback. No offset constants,
tables, assets, dead-ball ownership rules, pass timing or shot release gates
changed. Presentation-only draw direction remains separate from physical
attachment.

## Permanent proof

`dynamic_dribble_attachment_probe` runs 20,000 unforced CPU frames and checks
every live, ordinary mode-11 owner on base 9/11. For each observation, ball X/Y
must equal the exact `$87:B649/$B66A` offset computed from cached resources.
It also requires natural 9<->11 reversals, preserved-resource reversal
witnesses, and cases where the retired tick reconstruction chooses both a
different pair and a different attachment point.

The initial corrected run observed:

| measurement | count |
|---|---:|
| live dynamic dribble observations | 1,106 |
| natural 9/11 reversals | 24 |
| reversals preserving the published pair | 8 |
| reconstructed-pair differences | 1,070 |
| reconstructed X/Y attachment differences | 462 |

`tools/test_cpu_gameplay.py` no longer excludes bases 9/11 from the sustained
attachment check. The five existing visual hashes remain unchanged; this is a
between-anchor physical attachment correction, not a sprite or asset change.

## Reproduction

```powershell
.\build.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc'
.\tools\build_vector_probe.ps1 -Name dynamic_dribble_attachment_probe
.\build\dynamic_dribble_attachment_probe.exe .\build\nba95_assets.pak
```

The strict configured-start differential remains separate work. This proof is
an exact native routine-boundary and production-sequence check, not a claim of
whole-possession trajectory equality.
