# Reverse-engineering and asset tools

This directory contains the tools exercised by the build and regression suite,
the maintained headless smoke tests, asset extraction, coverage reporting, and
the current CPU reverse-engineering workflow. Completed one-off capture and
audit helpers are archived in Git history after their durable fixture is
checked in under tests/fixtures/.

## Full-suite execution

`build.ps1 -Test` runs every maintained gate and prints per-script timings.
Logs and `timings.json` are retained in the printed `build/test-runs/<id>/`
directory, including failures. Vector probes share one MSVC initialization;
the long CPU regression runs after the shorter route and compositor gates.
See [the testing workflow](../docs/testing.md) for coverage ownership and the
bounded trace decode cache.

The canonical graphics-WRAM lifetime probe exercises real game initialization,
scene transitions, new-match state, Tipoff binding, shutdown, and reinit:

~~~powershell
./tools/build_vector_probe.ps1 -Name game_wram_lifetime_probe
python tools/test_game_wram_lifetime.py --probe build/game_wram_lifetime_probe.exe --pack build/nba95_assets.pak
~~~

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

Replay the complete mode-two parent and its real production caller with:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_two_parent_vector_probe
python tools/verify_cpu_mode_two_parent_vectors.py `
  --vectors tests/fixtures/cpu-mode-two-parent-witnesses.json `
  --probe build/cpu_mode_two_parent_vector_probe.exe `
  --pack build/nba95_assets.pak
python tools/test_cpu_mode_two_parent_fixture.py `
  --vectors tests/fixtures/cpu-mode-two-parent-witnesses.json
~~~

The verifier pins 43 repeated native calls, 197 represented words, all 81
parent instruction starts, paths and child calls. It rejects malformed binary
probe output and partial input records, then checks the real
`nba_tipoff_update` rebound-state caller. The mutation test rejects fixture
value, type, shape, provenance, path, child, domain and ROM changes before
production replay. Capture scripts and raw vectors remain ignored; their exact
source and output hashes are retained in the fixture.

The compact mode-eight replay and its real caller-phase checks run with:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_eight_vector_probe
python tools/verify_cpu_mode_eight_vectors.py `
  --vectors tests/fixtures/cpu-mode-eight-witnesses.json `
  --probe build/cpu_mode_eight_vector_probe.exe `
  --pack build/nba95_assets.pak
~~~

The mode-ten receiver replay, preservation checks, and production scheduler
checks run with:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_ten_vector_probe
python tools/verify_cpu_mode_ten_vectors.py `
  --vectors tests/fixtures/cpu-mode-ten-witnesses.json `
  --probe build/cpu_mode_ten_vector_probe.exe `
  --pack build/nba95_assets.pak
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

Replay the mode-twelve shooter parent and its direct free-throw caller against
their compact native witnesses with the local ROM-derived pack:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_twelve_vector_probe
python tools/verify_cpu_mode_twelve_vectors.py `
  --vectors tests/fixtures/cpu-mode-twelve-witnesses.json `
  --caller-vectors tests/fixtures/cpu-mode-twelve-ft-caller-witnesses.json `
  --probe build/cpu_mode_twelve_vector_probe.exe `
  --pack build/nba95_assets.pak
~~~

The verifier also runs the authoritative-roster/stale-mirror launch case and
public `nba_tipoff_update` scheduler tests. Build the production executable
before the probe whenever production objects change.

Replay the mode-thirteen carried-ball close-finish parent against its compact
native witnesses:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_thirteen_vector_probe
python tools/verify_cpu_mode_thirteen_vectors.py `
  --vectors tests/fixtures/cpu-mode-thirteen-witnesses.json `
  --probe build/cpu_mode_thirteen_vector_probe.exe `
  --pack build/nba95_assets.pak
~~~

The verifier checks all 37 native entries across 144 represented words, the
authoritative `$093E` owner, and the production scheduler boundary.

Replay the mode-fourteen special-receiver parent against its compact native
witnesses, reject malformed fixture mutations, then exercise all supported
shot-table payload layouts and malformed local pack mutations:

~~~powershell
./tools/build_vector_probe.ps1 -Name cpu_mode_fourteen_vector_probe
python tools/verify_cpu_mode_fourteen_vectors.py `
  --vectors tests/fixtures/cpu-mode-fourteen-witnesses.json `
  --probe build/cpu_mode_fourteen_vector_probe.exe `
  --pack build/nba95_assets.pak
python tools/test_cpu_mode_fourteen_fixture.py `
  --vectors tests/fixtures/cpu-mode-fourteen-witnesses.json
python tools/test_cpu_mode_thirteen_pack.py `
  --probe build/cpu_mode_fourteen_vector_probe.exe `
  --new-pack build/nba95_assets.pak
~~~

The verifier checks all 51 native entries across 154 represented words, exact
owned paths and child calls, authoritative owner identity, and production
caller scheduling. The pack test checks the current eight-range payload and
derives temporary seven-range and five-range payloads from it; it does not
store ROM table bytes in the repository.

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
census. Product scope and remaining gaps live in STATUS.md. Durable system
workflow documents explicitly allowed by AGENTS.md, including
docs/cpu-logic.md, may also be maintained when they describe real production
dispatch and evidence boundaries.
