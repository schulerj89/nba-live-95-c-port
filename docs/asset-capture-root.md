# Read-only capture root for isolated builds

`tools/extract_assets.py --capture-root PATH` lets an isolated worktree read
existing capture evidence from another directory. Without the option, paths
still resolve under the extractor's repository `.analysis` directory. Existing
per-resource environment overrides retain precedence. Capture inputs are read
only; outputs continue to use the explicit output path.

The full extraction in `build/full-extraction-v1` uses the original ROM and
the primary checkout's capture tree, with the existing Rules opening/return
overrides selecting the audited publication capture. Its263-entry89438786-byte
pack matches the previous candidate byte for byte: SHA256
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
`equivalence.json` records both input-pack identities and per-entry comparison.
Only145/155 differ from the older main pack, as already expected from the Rules
publication correction; this path option introduces no resource-content change.

This is extraction reproducibility, not asset-pipeline completion. Captured PPU
schedules, remaining captured glyph resources and DSP trace playback retain
their previously documented limitations. The primary pack and desktop binary
are unchanged.

`build.ps1 -CaptureRoot PATH` now passes the same read-only root to extraction,
the extraction safety test and the existing native gameplay/intro checks.
Omitting it retains the repository `.analysis` default. The core safety test
still extracts both clean and copier-headered ROMs and compares all resulting
bytes, and still rejects the deliberately modified ROM. The first isolated
full-suite attempt failed because this test tried to read absent local capture
files; `build/core-safety-capture-root-v1.log` subsequently reached the unrelated
legacy debug-state assertion, proving both extraction cases completed.
