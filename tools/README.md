# Reverse-engineering and asset tools

This directory contains the tools exercised by the build and regression suite,
the maintained headless smoke tests, asset extraction, coverage reporting, and
the current CPU reverse-engineering workflow. Completed one-off capture and
audit helpers are archived in Git history after their durable fixture is
checked in under tests/fixtures/.

## Asset pipeline

Enable script file I/O in Mesen, then capture the verified US ROM inputs:

~~~powershell
./tools/capture_assets.ps1 -RomPath '<path-to-rom>' -MesenPath '<path-to-Mesen.exe>'
~~~

Extract and test the complete pack:

~~~powershell
./build.ps1 -RomPath '<path-to-rom>' -ExtractAssets -Test
~~~

Capture outputs belong under ignored .analysis/ directories. The pack contains
indexed graphics, palettes, tile maps, OAM resources, SPC/DSP state, and BRR
samples. RGB screenshots are comparison evidence and are not packed.

## Headless frame checks

The broad capture runner uses one identified executable, pack, and ROM:

~~~powershell
python tools/run_visible_smoke_checkpoints.py --help
~~~

Focused gameplay checks build or reuse the production executable, configure
the shortest route to the target state, press buttons where the route requires
them, and capture frames inside the renderer:

~~~powershell
./tools/run_dribble_smoke.ps1 -RomPath '<path-to-rom>'
./tools/run_hoop_smoke.ps1 -RomPath '<path-to-rom>'
./tools/run_oob_smoke.ps1 -RomPath '<path-to-rom>'
python tools/test_cpu_reaction_smoke.py --rom '<path-to-rom>'
python tools/test_tipoff_court_smoke.py --rom '<path-to-rom>'
~~~

Outputs stay under ignored build/ directories.

## Native capture and replay

run_differential.py drives controlled Mesen and C runs through shared field
schemas and reports the first mismatch. It does not claim whole-game parity
when the initial state differs.

The retained mode-two pipeline is the current working example:

~~~powershell
python tools/regenerate_cpu_mode_two_reference.py --help
./tools/capture_cpu_mode_two_role.ps1 -OutputDir '.analysis/cpu-mode-two-role'
python tools/normalize_cpu_mode_two_role_vectors.py --help
~~~

It and the permanent fixture verifiers follow the same workflow:

1. Capture a real native entry and exit into a new ignored directory.
2. Normalize only complete calls into a durable fixture.
3. Replay the fixture through the matching compiled production probe.
4. Reject malformed fixtures, missing outputs, unexpected writes, or changed
   instruction census data.
5. Exercise the production caller separately through a runtime or smoke test.

Ghidra scripts under tools/ghidra/ label the matching ROM routines and
regenerate listings. Generated recomp C is a second structural reference;
native Mesen state remains the behavioral oracle.

## Local launcher

After building the executable and asset pack, recreate the desktop shortcut
with the selected verified ROM:

~~~powershell
./tools/create_shortcut.ps1 -RomPath '<path-to-rom>'
~~~

## Regression entry points

The configured suite runs harness integrity checks, the current CPU native
replays, and complete product-route regressions:

~~~powershell
./build.ps1 -RomPath '<path-to-rom>' -AssetPack 'build/nba95_assets.pak' -Test
~~~

Superseded one-routine probes and wrappers remain available in Git history;
their durable native fixtures stay under tests/fixtures/ for provenance.
Useful focused tests include:

- test_intro_sequence.py, test_title_pipeline.py, and
  test_setup_transition.py for the frontend;
- test_team_select.py, test_player_setup.py, and test_player_intro.py for
  pregame flow;
- test_tipoff.py, test_cpu_gameplay.py, and test_gameplay_audio.py for
  gameplay;
- test_project_census.py for generated evidence freshness and ledger
  constraints.

Tests with golden images state whether they protect inspected C output or
native frame parity in their source comments.

## Coverage reports

Regenerate captured-address progress:

~~~powershell
python tools/progress.py --write docs/progress.md
~~~

Regenerate the conservative full-ROM census after a fresh headless Ghidra run:

~~~powershell
./tools/ghidra/Run-FullRomCensus.ps1 `
  -RomPath '<path-to-rom>' -GhidraHome '<path-to-ghidra>' -JdkHome '<path-to-jdk-21>'
~~~

The census is a recursive lower-bound disassembly seeded from native
execution, verified routines, source provenance, recomp functions, and SNES
vectors. Undecoded ROM bytes may be data or undiscovered code. Neither report
is a game-completion percentage.

The checked-in documentation artifacts are limited to
docs/verified-routines.json, docs/progress.md, and the JSON/Markdown full-ROM
census. Product scope and remaining gaps live in STATUS.md.
