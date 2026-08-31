# Main value span restoration

This corrects a port regression in the existing captured-glyph projection.
It does not fix an original-game bug or complete the native font writer.

The resumed implementation correctly preserved nineteen-pixel shadow cells
and composed overlapping rows from bottom to top, but dropped the earlier
per-word horizontal source bounds. Both canvas and renderer paths copied the
whole 110-pixel source cell. The correction clears that whole destination
while sampling only each measured glyph-plus-shadow width. Default words now
have explicit widths too, because fresh defaults differ from the asset capture.

The historical stale-tail test was ineffective: it copied an empty tile from
tile row 8 (screen sample y71). The corrected mutation copies actual Season
ink from tile row 9 to x216. Its reference is the same current CLI journey
with the pristine pack, rather than an old Simulation/3-minute whole-frame hash.

Retained owner evidence under `build/`:

- `span-attribution-v2/report.json`: fourteen valid Main journeys have identical
  complete RGB output before and after the correction. The deliberate corrupted
  pack adds 48 pixels with the old executable and zero with the corrected one.
  Exact commands, file/source identities, BMPs and logs are retained alongside.
- `config-span-v1/`: all 40 full Main VRAM canvases and 7 full Rules canvases
  match the unchanged native witnesses, each 65,536 bytes. Configuration gates
  also pass 730 stable checkpoints and 1,770 adjustment observations.
- `rules-open-span-v1.log`: all 147 native opening RGB/PPU frames pass unchanged.
- `setup-transition-next-v1.log`: the monolithic legacy regression gets past
  the span guard and fails at initial Setup frame162's historical default-state
  hash. That separate migration remains unresolved; this is not a full-suite pass.

The first diagnostic mutation attempt is retained in `span-attribution-v1/`:
its negative control correctly rejected the ineffective old test construction.
Neither attempt modified native evidence or the canonical asset pack.

Independent review rejected the first correction: it computed a destination
tile address from the source map. When clearing the poisoned source tail,
that address aliased an in-span glyph and erased sixteen raw-canvas bytes.
The prior implementation's raw canvas did not suffer this particular error;
the first span correction introduced it, despite leaving RGB correct. Rejected
source and counterfactual outputs are retained in the auditor's
`build/span-audit-v1/`.

The revised correction samples source colors only within the word and uses
the destination canvas's own tile map for every write and clear. The new
`test_setup_main_span.py` checks full RGB and the complete 65,536-byte raw
canvas with the same deliberate source-map mutation. It is wired into the
full build test path. Fresh verification and independent re-review are pending.
Final independent review accepts revised freezev2 for the bounded span scope;
see `completion-main-span-independent-audit.md`. All18 independent cases pass,
including14 distinct actual row/value journeys, RGB poison resistance and raw
canvas poison resistance. The owner's14 journeys also cover distinct values:
the retained quarter telemetry for0/1/2/3Right presses is3/0/1/2. Main values
wrap. An earlier audit comment incorrectly said Right clamps at12minutes;
this was checked against those logs and the native Main adjustment contract.
The independent run uses Left and reaches the same four quarter values.
The47 native canvases,730configuration checkpoints,57391input frames and1770
adjustment observations pass again. The owner's147native opening frames also
pass separately. Early highlight/initial-default migration, reentry and the
full legacy suite remain unaccepted. Main, the primary build, and desktop
shortcut remain unchanged.
