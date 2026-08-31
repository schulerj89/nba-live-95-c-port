# New Exhibition match reset

The August 30 ownership audit found a reproducible second-match freeze.
Postgame returned to Game Setup with `session.match.flow_state=FINAL`, raw
period 5, a confirmed final marker and the previous match's timeouts/lineups.
The next normal frontend journey called `nba_tipoff_init`, which correctly
preserved session lifecycle state for its caller; `nba_tipoff_update` then
immediately returned through the FINAL gate on every tick. Existing closure
probes created a fresh `NbaSession` and invoked scenes directly, so they never
exercised this route.

## Ownership and native references

`nba_session_begin_match` resets match-owned state while preserving the
complete configuration, selected teams and Player 1's selected side. It resets
scores, elapsed host ticks, period, timeout counts, roster order/eligibility,
lineup, pause state and final/presentation state. Process startup calls the
same initializer. The normal Game Setup Exhibition confirmation calls it
before entering Team Select. Browsing Setup or returning from Rules/Options
does not reset a match. Quarter/overtime/timeout resumes never call it.

The concrete native distinction is `$86:DBE8` (`STZ $0926`) during new-match
initialization versus `$86:DD2D-$DD44`, which reads the existing `$0926` and
selects a later-period clock. The original ROM bytes at `$86:DBE8` are
`9C 26 09`; the focused verifier checks them independently.

The Ghidra listing is
`.analysis/gameplay85-closure-ghidra/gameplay85_bank86_listing.txt`, around
`$86:DBD1-$DC6B`. Fresh readable recomp was generated with the local
`tools/snesrecomp-source-v0.2.0-alpha/recompiler` into
`.analysis/new-match-reset-20260830/new_match_bank86.c`, function
`NewMatchClockAndPeriod_M0X0`, `$86:DBD1-$DBEA`. This intentionally bounded
reference ends before `$DBEB`; its unresolved continuation is not treated as
executed native evidence or linked into the port.

`tests/fixtures/new-match-native-start.json` retains period, scores, timeout
counts and both twelve-slot lineup permutations from the independent original
ROM first-court WRAM snapshot in
`.analysis/differential-release-final-cpu/baseline.wram`. Its manifest/hash and
the pregame team/CPU-selection injection are recorded. Native projection:
period 0; scores 0/0; timeouts 7/7; both roster orders
`[2,0,1,3,4,5,6,7,8,9,10,11]`.

Host-only lifecycle enums and availability flags have no direct one-word ROM
counterpart; their reset is the portable new-match state contract. This fix
does not claim full native roster-selection semantics for unimplemented modes,
or a newly captured natural second-match sequence.

## Verification

`tools/new_match_runtime_probe.c` seeds a completed C match with nondefault
scores, empty timeouts, reordered/unavailable players and stale pause state.
After that explicit controlled seed it uses the real postgame return, input
edge publication and frontend dispatcher through Setup, Team Select, Player
Setup and introductions into Tipoff. Both selected sides, different teams and
nondefault configuration are preserved. Every new Tipoff must begin period 0
with the configured regulation clock and advance 60 real update calls.
Merely browsing Setup must leave the prior final state intact.

`tools/verify_new_match_reset.py` parses the native fixture values and compares
the actual production-reset output to them. It also checks the ROM hash and
new-match period-clear opcode. With `--native-snapshot`, it validates the
snapshot hash and re-extracts every represented native value; a modified
fixture result cannot silently pass.

```powershell
./tools/build_vector_probe.ps1 -Name new_match_runtime_probe
python tools/verify_new_match_reset.py `
  --fixture tests/fixtures/new-match-native-start.json `
  --probe build/new_match_runtime_probe.exe --pack build/nba95_assets.pak `
  --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' `
  --native-snapshot .analysis/differential-release-final-cpu/baseline.wram
```

The pre-fix production caller failed this probe, retaining period 5, scores
103/99, timeouts 0/1 and the reversed unavailable lineup after Exhibition
confirmation. See `.analysis/new-match-reset-20260830/before-caller.log`.
Fresh `/W4 /O2` build and relink on August 30 passed both production return
journeys, native snapshot/run-manifest checks and the exact projection. Seven
synthetic verifier tests reject duplicate keys, invalid integers/booleans,
malformed dimensions/permutations/provenance and changes to all 29 projected
scalar values. A separate corrupted-score fixture failed against the original
snapshot before the C process was started. Rebuilt lifecycle initialization
checks passed all four regulation and four overtime settings, and rebuilt
timeout/resume freeze/restoration tests passed. Logs and exact source/binary/
pack hashes are in `.analysis/new-match-reset-20260830/validation.json` and
`after-caller.log`.

These are separate claims: original first-court initialization projection,
ROM/recomp control-flow evidence, and C-only repeated frontend integration.
Neither the 6,000-call liveness ceiling nor 60 ticking calls is a native timing
tolerance. No framebuffer, audio, whole-state or natural second-match parity
is asserted, and no instruction-coverage credit is added.
