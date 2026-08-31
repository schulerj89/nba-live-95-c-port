# Independent Main value span audit

**PASS for Main span freeze-v2's bounded source/destination correction and
settled-value regression guards.** Freeze-v1 was rejected for a newly introduced
raw-canvas alias defect. Early Setup transition/highlight behavior, Rules
reentry and the monolithic legacy test remain outside this approval. No old
frame hash or native fixture was rebaselined by this audit.

The final owner source SHA256 is
`cc6a53b5ff90612b430f6043432138ef897acf01355ebeb9435fcaf41a3f0f65`.
Every file declared in `completion-owner/build/span-freeze-v2.json` was checked
before the source and test were copied into the auditor's
`build/span-audit-v2`. The candidate Setup module was compiled separately and
linked with the independently built freeze-v4 driver/game objects; no owner
objects were used and no auditor production source was replaced. Fresh canvas
and configuration probes use that same candidate module. Auditor executable:
`0079a54a908ab2513ae25d6614344ef5a62dafc6e1bd5fbfd8203fa930bccc85`.

The change preserves the nineteen-line glyph/shadow cells and bottom-to-top
composition. Source reads are limited to each observed value width, while
clears cover the complete old110-pixel destination. The revised raw path
samples only an in-span source pixel through `nba_snes_sample_bg`, then writes
that color or zero through `setup_write_bg3_cell_pixel`, which uses the target
canvas's tile map and flips. An irrelevant source-map tail can no longer
choose which destination glyph data gets cleared. The renderer independently
restores its entire background rectangle and copies only the source span.

The native font owner remains `$81:9FD4`, with upload at `$81:A1EE`. This is a
bounded reuse of observed proportional-glyph pixels, not a new translation of
the native font writer. Exact comparison against the unchanged natural Main
canvases constrains the widths and shadow overlap for all14 distinct values.
It does not establish equivalent construction timing or transitional highlight
placement. No original-game bug was changed: the overbroad host copy and v1
source-address clear were host projection defects.

Independent verification on the rebuilt candidate:

| Check | Result | Scope |
| --- | --- | --- |
| Full natural Main VRAM | 40/40 | Every65,536-byte canvas exact; corpus covers all four mode, three style, three difficulty and four quarter values |
| Full natural Rules VRAM | 7/7 | Every65,536-byte canvas exact; existing factory/multiple-off/reentry settled cases |
| CLI configuration/input replay | 730 checkpoints,57,391 input frames | Existing exact native state/input comparisons; not full constructor parity |
| Adjustment observers | 1,770 observations | Existing real caller boundary comparisons |
| Controlled before/after RGB | 14/14 distinct row/value journeys | Complete RGB unchanged for every value; telemetry confirms the requested value was actually reached |
| Effective poison negative control | Old48 changed RGB pixels | Copies actual Season ink from tile row9 into the out-of-span x216 tile |
| Repaired poison behavior | New0 changed RGB pixels and0 changed raw bytes | Same current journey with pristine/poisoned pack; full canvas comparison |

The fourteen independent journeys use Left for quarter choices descending from
factory12 minutes. Main values wrap, so the owner's Right journeys also cover
all four quarter choices: `span-attribution-v2/after-3-{0,1,2,3}.log` records
quarter values `[3,0,1,2]`. The earlier audit statement that these clamped and
lacked distinct coverage was incorrect; source and actual telemetry confirm
the owner's fourteen journeys were distinct. All18 independent cases pass
with the same test logic that exposed the v1 raw defect; only the incorrect
explanatory comment was corrected afterward.

The original tail test used an empty tile from row8 and was not an effective
negative control. The owner corrected it to row9 and compares the poisoned
pack with the same pristine current journey rather than an unrelated historical
Simulation/three-minute RGB hash. I inspected the new `test_setup_main_span.py`
and its build integration: it checks both full RGB and full raw VRAM; the
independent test additionally retains an effective old-executable negative
control. The new test source SHA256 is
`630c3d75e9372a9129128689d304a34536fa76f9f956b79a23d8edb667e273ad`.
Other existing migrations in `test_setup_transition.py` were not re-approved
wholesale as part of this small review.

The rejected first correction passed valid canvases and the RGB poison guard,
but cleared using the source map beyond the span. The poisoned x216 tile
aliased an in-span Season tile, erasing16 bytes at `$8D70-$8D7F`. The older
unbounded-copy raw path did not suffer this particular defect. Its unchanged
source, binaries, outputs and17/18 failed report remain in
`build/span-audit-v1/independent-controls-v2/report.json`. The first local test
runner attempt used an incorrect log prefix and was discarded; after correcting
the runner to read the actual `[SETUP MAIN TEST]` line, the complete failing
v1 and passing v2 case sets were run to new directories. No successful result
was inferred from that incomplete attempt.

Final auditor evidence:

- `build/span-audit-v2/independent-controls/report.json`: all18 checks, exact
  commands, full output hashes and input executable/pack identities.
- `build/span-audit-v2/main-canvases/report.json` and
  `rules-canvases/report.json`:47 native raw comparisons.
- `build/span-audit-v2/headless-replay/report.json` and
  `adjustments-{primary,input}.json`: input/configuration results.
- `build/span-audit-v2/freeze.json`, source copy and `compile.bat`: exact
  candidate and standalone relink. The initial v1 compile command omitted a
  Windows host object and failed linking; its corrected command and complete
  successful build were used for all comparisons. No failed build was counted.

The primary ROM SHA256 remains
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`,
and the candidate pack remains
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
The audit reads the owner's deliberate poisoned pack; neither original input
was changed. The owner's147-frame opening rerun is separate evidence and was
not repeated here. Early Setup frame162, full transitional highlight behavior,
Rules reentry and overall game completion remain unaccepted by this review.
