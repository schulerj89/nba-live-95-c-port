# Independent headless input/configuration acceptance

**PASS for the frozen headless input driver and bounded configuration replay.**
2026-08-31. This does not approve the whole unfinished WIP integration, complete
transitions, gameplay consumers, audio, persistence, human controllers or the
game as finished.

Accepted owner freeze:
`.analysis/worktrees/completion-owner/build/headless-input-freeze-v4.json`.
The auditor copied only the exact frozen `src/main.c` and
`tools/test_headless_input.py` into `work/completion-auditor-20260830`, verified
their SHA256 against the freeze, and freshly built **all** production objects
in that worktree. Dedicated configuration probes were then linked only against
those current local objects. No owner object/executable was reused.

| Accepted input | SHA256 |
|---|---|
| `src/main.c` | `0f07289bad60ea4371decc4a52bddb38a8677e9c2441150f9b2f4602c23de5f1` |
| `tools/test_headless_input.py` | `b0892ae0cff3575b1b4b6c74e88cdba3e1c875c4b7e760cdf403ff0854b25bcb` |
| Independent `tools/test_headless_input_integrity.py` | `bfa18ee0d34f018279e0f301e4a850f2c0e215c2617b5da153e2dece2e326bd0` |
| Auditor executable | `891dc6473d6e1805b5660f79722f58e151c84e51c827089f1aad9ec2f80cb5a0` |
| Candidate pack,89,438,786 bytes | `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df` |
| Original ROM,1,572,864 bytes | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |

## What was independently inspected and replayed

The auditor personally inspected original ROM bytes, Ghidra's correctly sized
`$81:AB58-$AC52` producer/consumer, `$81:BFAA-$C00A` presets and factory
`$81:C19A` initialization, bounded recomp `bank81-with-repeat.c`, and the real
C `nba_game_input_update` / Setup producer / menu dispatch callers. Source
identities and the additional default/Custom byte checks are recorded in
`completion-config-inventory-20260830.md`. All ten raw native configuration
journeys were freshly reloaded and compared against their compact actions,
states, events and manifests, validating361 retained source identities.
Native expectation bytes were not changed or regenerated from C.

Fresh final checks in the auditor worktree:

| Gate | Actual result | Scope |
|---|---|---|
| Production CLI native replay |730 stable checkpoints;57,391 exact input frames; zero differences | Seven normal-input native schedules passed through the actual CLI and C menu callers. Includes working/committed/Custom values, cursor and source-interpreted page. |
| Existing native configuration suite |730 stable checkpoints,1,770 adjustment entry/exit observations,40 Main and7 Rules canvases | Independently rebuilt probes; preserved immutable native references. Canvas comparisons cover full64KiB VRAM but do not prove upload/scanout timing. |
| Existing protocol/evidence tests |30 PASS; standalone C menu-input regression PASS | Provenance, malformed protocol and deterministic producer contracts; not independent ROM parity by themselves. |
| New independent protocol tests |7 PASS | Every missing column, added/duplicate column, malformed row, missing/extra/duplicate/reordered frame, input population, and every latch across held/changed/released full words. Synthetic protocol doubles, explicitly not game evidence. |
| Updated driver cases |5 automatic scripts,10 malformed schedules,6 flag guards PASS | Factory, configured Custom, Rules clamp, two repeated visits, Main-edit→Rules; expected parser errors verified rather than accepting unrelated process failures. |
| Additional independent final CLI cases |13 PASS | Preflight conflicts, working/committed telemetry, fixed title frame, Team gap, three complete visits, parser boundaries and full-word latches. |

The final extra cases verify:

- Conflicting Main-confirm/menu, fixed-title/menu and Setup-down/menu commands
  exit1 before opening an input trace or issuing match confirmation.
- Style editing reports working1 and committed0 separately until the actual
  menu commit. Fresh defaults remain Arcade/Rookie/12 minutes; the optional
  Simulation/3-minute profile navigates through actual Down/Right/Up inputs.
- `--title-only --title-press 40` emits exactly one Start at loop frame40
  (one-based trace step41). This is a driver timing check, not title parity.
- Team action gap3 produces presses180,184,188, preserving the prior
  gap-plus-one interval while counting the release frame as an idle frame.
- Three requested Rules visits produce exactly three A openings, three Start
  returns, three page0→1 transitions and three page1→0 transitions. The
  two-visit permanent test now counts visits; final Main alone is insufficient.
- Adjacent identical explicit words remain held. `FFF0` preserves all twelve
  buttons. Changed nonzero words publish only the true pressed/released bits.
  Final lines without a newline are accepted; total-frame overflow,8193
  records and embedded NUL input are rejected by the intended parser.

Additional v1 parser checks also covered comments/whitespace, decimal/hex
overflow and overlong lines; those paths did not change in the final version.

## Defects found before acceptance

Initial freeze v1 was rejected despite its730-checkpoint PASS:

1. Accepted Main-confirm+submenu flags crashed with Windows access violation
   `0xC0000005`; final telemetry dereferenced an inactive Setup scene union.
2. Missing `--input-script` / `--input-trace` arguments were silently ignored.
3. Main edit telemetry reported committed values as though they were edited
   working values.
4. A fixed-frame title pulse could silently disappear when mixed with an
   automatic input's release frame.
5. The new trace reader accepted a missing `delay` column and an extra invented
   column. Its schema now requires every column in exact order, and its report
   binds the verifier and imported comparison/normalization source hashes.
6. The automatic repeat test could pass after one visit; it now checks exact
   pulse and page-transition counts.

Intermediate fixes were independently replayed. Review then caught the conflict
guard placed after execution and the Team gap countdown skipping release
frames. The accepted freeze moves rejection before trace opening/stepping and
restores the original Team gap interval. All original defect commands were
replayed after these fixes. None of these host defects was attributed to an
original-game bug.

Confirmed native quirks remain intact and documented in `nba_menu_input.c`
and the native configuration contract: release preserves pending/delay/speed,
changed nonzero words preserve fast-repeat state, submenus compare complete
words, Rules clamps while Options wraps, clamped Rules adjustments mark Custom,
and separately retained factory Custom differs from Simulation. The user's
instruction to preserve/comment original bugs authorizes no reinterpretation
of unresolved divergences as native bugs.

## Reproduction and retained evidence

Run in `.analysis/worktrees/completion-auditor` with the original ROM and the
explicit candidate pack (not a new extraction or a stale/default pack):

```
.\build.ps1 -RomPath '<ROM>' -AssetPack '<candidate pack>'
python tools/test_headless_input.py --exe build/nba95_port.exe --rom '<ROM>' --pack '<candidate pack>' --output '<new directory>'
python tools/test_headless_input_integrity.py
.\tools\run_setup_config_checks.ps1 -RomPath '<ROM>' -AssetPack '<candidate pack>' -OutputDir '<another new directory>'
```

Final local evidence: `build/headless-auditor-build-v4.log`,
`build/headless-auditor-replay-v4/report.json`,
`build/headless-auditor-config-v4/`,
`build/headless-auditor-config-v4.log`, and
`build/headless-auditor-extra-v4b/report.json`. The extra report retains exact
commands and source identities. The first extra-v4 runner finished its checks
but failed serializing source identities because its own Python list index was
wrong; it was not counted as a passed run. The complete identical case list was
rerun to new directory `extra-v4b` after correcting only that runner expression.
Earlier rejected-run logs and schema mutation proofs remain immutable in
`build/headless-auditor-extra-v1/` and intermediate replay directories.

The CLI native gate starts C directly at Setup with400 idle ticks while native
evidence starts at boot. Its57,391 comparisons own input words/edges, not every
CPU/PPU/APU state. The1,770 observations own specific adjustment boundaries;
the730 stable checkpoints do not cover all input consumed during divergent
constructors. Other CLI combinations, cross-platform builds and complete
gameplay journeys are not certified by this audit. Six historical native
manifests retain only pinned indirect successful-exit evidence; no numeric
exit0 was retroactively fabricated. Current normal/repeated transition phase,
asset provenance beyond the claimed canvases, audio, runtime options and disk
persistence remain separately incomplete.
