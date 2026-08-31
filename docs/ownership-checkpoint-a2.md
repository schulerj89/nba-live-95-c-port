# Ownership checkpoint A2 — indexed intro resources

Starting revision: `e1bc0d4db83c1e19998e025cf653650aedc62437`.
Date: 2026-08-30. **Whole-game and complete-intro acceptance remain FAIL.**
This checkpoint repairs the production artwork source and a bounded renderer;
it does not complete intro scheduling, human controls, or the remaining game.

## Changes and independent evidence

- Replaced handwritten license rows and screenshot-derived intro graphics
  IDs1–6/70–74 with pack75/76: original indexed tiles, palettes, OAM, font and
  strings. Runtime never opens capture files or calls an emulator. Some static
  format-$30 resources remain attested native-memory extractions; independent
  decompressor translation is still open.
- Corrected EA integer matrix products, tilegroup publication, fixed-object
  handoff and palette stepping. Independent native RGB now matches303
  consecutive motion frames0–302, all57,344 pixels each. These include static
  holds and are aligned by renderer phase, not whole cold-boot time.
- Shared original 2bpp font rasterization produces five exact production text
  samples and31 native-brightness rasters. One extra license black output uses
  a clearly labeled native forced-blank reference, not native brightness0.
- Removed the embedded license fallback and require both indexed resources
  at startup. Payload/header/metadata validation fails malformed packs.
- Replaced selected C intro hashes and optional PNG tolerances with independent
  exact-pixel evidence. Added strict provenance/geometry/phase rejection tests
  and an independently authored seven-group frame-integrity suite.
- Added tracked, immutable-script capture tooling with private Mesen settings,
  saves and explicit per-process environments. A fresh run reproduces1,804
  relevant native files and both resource payloads exactly.

Source owners, state mappings, capture identities and explicit exclusions:
`intro-indexed-resources.md`. Independent PASS for this bounded scope:
`ea-indexed-independent-audit.md` and `intro-text-independent-audit.md`.
The auditor independently compiled C, read ROM/Ghidra/recomp, rebuilt the two
resources, validated1,807 original artifacts, inspected images and tested
malformed evidence. It did not merely approve the implementer's report.

The full normal extractor produces263 entries /88,646,647 bytes, pack SHA256
`c7b90d9347c257e0746da7a6d5595e603ffd9d3a026666fe6e62c4f483e75a92`.
All261 common entries remain metadata/byte-identical to A1. Directory ordering
alone differs from the separately repacked, independently audited candidate.
The tested final executable SHA256 is
`dc29aec6fd1ee81f30e899f29984dd6f3e51a36c2edd50916247efc8b7179651`.

## Regression attribution and release validation

The complete `build.ps1 -Test` test list passed against the executable/pack
above. It was completed across the original run and two exact remaining-test
blocks after the fixture fixes below; no production C source changed between
those runs and no gate was omitted. Logs are
`build/intro-checkpoint-full-suite.log`,
`build/intro-checkpoint-suite-remainder.log`, and
`build/intro-checkpoint-suite-final-remainder.log` (final block exit0).
The C checks include63,800 lifecycle frames,48,000 multi-team updates,
6,000 closure updates and63,800 CPU gameplay/telemetry frames. These are
deterministic C endurance/regression checks, not synchronized native matches.
All existing focused native projections, UI/asset/audio checks, new exact
intro gates, integrity tests and census accounting passed.

The previous A1 executable and pack are preserved under
`.analysis/checkpoint-a2-20260830/previous-a1/` for replaying regression
attribution. The C Port desktop build and pack are refreshed to the tested
identities above; its shortcut uses the original ROM and this pack. The
separate Recompiled shortcut remains untouched.

The test list initially stopped on two fixture assumptions, both resolved:

1. F12's displayed directory index/count changed272→263. The retained A1
   executable reproduced all six previous full/reduced debugger hashes.
   Independent fresh comparison of all12 old/current captures found changes
   only in metadata x8–62,y19–25. Six C-only debugger hashes were updated with
   that attribution; every other pixel and common asset stayed unchanged.
2. Formation/play-control parser fixtures previously launched the game with
   one graph asset. They now include real required75/76 boot resources in both
   valid and corrupt cases. The original corrupt graph and exact-error checks
   remain unchanged; independent fresh replay passes.

Core-safety's old duplicate-ID fixture also had invalid bitmap metadata and
could fail before testing duplication. It now uses individually valid entries
and requires the actual duplicate-ID error. Missing boot assets are tested
separately, and Setup-audio negative fixtures include the graphics prerequisites
so they reach the failure they claim to exercise.

Root inspected consecutive E/A/SPORTS/flash/hold image sheets and five text
pairs. The303-frame paired video and full pixel report are under
`.analysis/intro-exact-20260830/final-visual-review/`. The original's large
zoomed colored cells are preserved; no invented fade, delay or crop was added.
These files are evidence only and remain outside Git and the asset pack.

## Remaining release failures

- License construction and wait count; the second legal wait/input loop;
  legal→EA resource/audio publication; full EA hold, input and title handoff.
- Original intro audio commands/sequencing. Asset7 remains an assembled set of
  ROM BRR samples at incorrect host cue times, not a verified native sequence.
- Rules reentry work/resource scheduling, partial DMA visibility and Options
  construction. Separately audited configuration and resource patches still
  require combined root integration and tests.
- Native match/team initialization, dynamic HUD, ordinary human ownership,
  shared gameplay/audio RNG and event consumption, every runtime rule/option
  consumer, full match presentation, retail modes and persistence.
- Strict whole-game differential:62/449 launch words differ, zero matching
  checkpoints. No later trajectory parity is inferred from this bad baseline.

Current address accounting is29,438 observed positions;29,091 have provenance
comments (98.8%) and11,529 intersect eligible evidence (39.16%). Removing
broad old intro comments reduced the documented count by347. No verification
credit was added to restore a percentage. The Ghidra count remains11,526 of
60,346 decoded starts (19.10%), with known context limitations. These metrics
exclude uncaptured/unrecognized behavior and do not measure game completion.
