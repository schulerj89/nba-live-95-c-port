# Regression workflow

Use the focused routine replay and production-caller tests while implementing
a change. Run the full suite after code and review are stable:

```powershell
./build.ps1 -RomPath '<local ROM>' -AssetPack 'build/nba95_assets.pak' -Test
```

Each run prints an ignored `build/test-runs/<id>/` directory. It retains each
Python gate's output and updates `timings.json` after every gate, including a
failing gate. The report records elapsed seconds and exit code; it is timing
evidence, not a cache of passing results. The runner still stops on failure.

All maintained gates remain enabled. Shorter route, asset, safety, and
compositor checks precede the expensive 63,800-frame CPU regression so an
early failure does not waste that run. Every CPU frame, gameplay assertion,
native fixture, liveness limit, and image expectation remains checked.

## Work shared within a run

`tools/build_vector_probe.ps1 -Name <name>` still builds a single probe. The
full suite supplies all six probe names together, initializes MSVC once, and
compiles each probe against the current production objects. Any compilation
failure stops the batch. Production objects must be built first; no stale
object or executable cache is introduced.

The core-safety gate runs its clean-ROM and copier-headered-ROM extractions
concurrently in two separate processes. Each has its own pack and provenance
filenames; shared captures are read-only. Both must succeed before the same
complete byte-for-byte pack comparison runs. This uses two extraction workers
instead of one, without replacing either extraction or its assertions.

`JsonlRows` in `tools/test_cpu_gameplay.py` decodes each JSONL row once into a
private temporary binary spool. Subsequent passes read that spool instead of
reparsing the large JSON trace. Memory remains bounded to row offsets and an
eight-row decoded cache. Default iterators retain independent snapshots and
read positions; the gameplay verifier opts into shared read-only rows as
described below. The spool is created from the current trace, never accepted from an
external cache, and removed on success or failure. The current trace requires
about 636 MiB of extra temporary disk space. `tools/test_jsonl_rows.py` checks
sequence behavior, concurrent iteration, malformed input, and cleanup.

The default 5,330-frame lineup-to-Tipoff handoff belongs to
`tools/test_player_intro.py`, which checks the scene and transition image
hashes. `tools/test_tipoff.py` no longer reruns that same journey just to check
the scene label; it retains its own tip-off phase and selected-home checks.

## Measured runtime

September 6, 2026, on the development Windows machine:

| Check | Before | After |
| --- | ---: | ---: |
| Six probe builds | 21.53 s | 3.43 s |
| Full retained-trace CPU verification | 801.44 s | 361.83 s |
| Core safety, including both extractions | 206.46 s | 97.88 s |

Both CPU measurements passed against the same 63,800-row trace, executable,
ROM, and pack, including all gameplay checks and five RGB renders. These are
observed stage timings, not a promised total-suite time or a sum of concurrent
benchmarks. Future runs record their own timing report.

## Remaining measured opportunity

The Setup transition test takes about 35 seconds and starts the game 137
times. Many checkpoints share a journey but require different image/debug
outputs. Combining those safely needs selected-frame capture support in the
headless runner. They remain enabled rather than dropping checkpoint coverage
or conflating different input schedules. The new timing reports make this
next target visible without repeated manual profiling.

ROMs, packs, images, raw traces, and binary spools stay outside Git. Retaining a
trace for diagnosis does not by itself authorize reusing a test result after
the executable or other relevant inputs change.

## Further Python optimization

The sustained-gameplay analyzer streams JSONL source lines and retains only
the fields consumed by its analysis. It no longer holds the complete source
text, split lines, and unused actor telemetry alongside its working rows.
The main gameplay verifier still checks the complete telemetry separately.
Every analyzer check and reporting function is unchanged; missing fields
needed by a future check cause a failure instead of an implicit skip.

`tools/test_cpu_trace_analyzer.py` compares complete reports and exit status
for full and compact rows. It covers passing play, stationary teams,
overlong dead-ball sequences, detached balls, retained-OBJ mismatches,
missing ball modes, optional scheduler data, and malformed JSON. It runs in
the normal full suite. The complete retained 63,800-frame report was also
compared byte-for-byte before and after the loader change.

On September 6, 2026, this analyzer ran in 25.22 seconds before and 10.21
seconds after on the same retained trace. These stage measurements exclude
capture and are separate from the complete CPU regression comparison.

The main verifier enables `JsonlRows(..., read_only=True)`. Iteration and
indexed look-behind share the existing eight-row decoded cache, so adjacent
scans avoid decoding the same frame twice. This is a caller contract: returned
dictionaries and nested values must not be mutated. The default remains
independent iterator snapshots for callers that need them. The verifier and
its helper checks read trace values without modifying them.

The loader tests count actual pickle decodes over more rows than the cache
can hold, checking adjacent iterators and indexed look-behind alongside cache
bounds, default snapshot independence, ordering, and temporary-spool cleanup.
On the same 63,800 rows, adjacent iteration fell from 6.60 to 3.42 seconds
(127,598 to 63,800 decodes); iteration with indexed look-behind fell from 6.70
to 3.46 seconds. Sequential iteration was essentially unchanged at 3.34 versus
3.40 seconds. No frames, assertions, liveness thresholds, or image checks were
removed.


A fresh sequential comparison of the complete retained-trace CPU regression
passed in 223.19 seconds before and 159.66 seconds after (28.5% less
wall time). Both used the same 63,800-frame trace, executable, ROM, and pack,
including whistle verification, every gameplay assertion, sustained analysis,
and all five RGB renders. Output was byte-identical. Capture time is excluded;
this is an affected-gate comparison, not a new timing for the entire suite.
Earlier timing figures reflect different machine load and should not be used
as the baseline for this second optimization.
