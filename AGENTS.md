# Repository instructions

These instructions apply to all work in this repository.

## One subroutine per iteration

- Implement, port, or fix exactly one subroutine at a time. Identify its native
  ROM address or C function and the behavior being changed before editing.
- Keep each iteration limited to that subroutine and the directly necessary
  caller integration, tests, and evidence updates. Do not bundle unrelated
  subroutines, refactors, or cleanup into the same iteration.
- Add or update executable tests covering the subroutine's behavior, relevant
  branches, and boundary cases. For native parity work, use native entry/exit
  evidence and replay it through the production implementation; exercise the
  production caller when integration changes.
- Build and run the focused tests and relevant regression checks. Fix failures
  before committing. Record the commands, results, and any remaining limits in
  the commit message or accompanying maintained evidence. Missing prerequisites
  or skipped tests do not count as passing verification.
- Commit and push each tested subroutine as its own commit before starting the
  next subroutine. If validation, commit, or push is blocked, resolve or report
  the blocker rather than accumulating additional subroutine changes.
- Documentation-only changes do not require artificial runtime tests; review
  the diff and check formatting before committing and pushing.

## Never commit assets

- Under no circumstances commit or push game assets, asset packs, ROMs, extracted
  asset data, graphics, audio, screenshots, or capture outputs. This prohibition
  also applies to test fixtures and to assets embedded as source arrays, encoded
  text, or other representations. Never force-add ignored asset files.
- Runtime code and tests must reference the user-supplied ROM-derived asset pack
  through the existing asset-loading interfaces. The default local pack is
  `build/nba95_assets.pak`; use `--assets` or the build script's `-AssetPack`
  option to select another local pack.
- When new resources are needed, update extraction/loading code and reference
  the resulting pack entries. Keep the generated pack and raw captures in
  ignored local directories such as `build/` or `.analysis/`.
- Tests may commit code, non-asset behavioral state vectors, resource identifiers,
  and expected hashes. Any asset bytes needed by a test must come from the local
  asset pack, never from a checked-in copy.
- Before every commit, inspect the staged file list and diff to ensure only the
  intended changes are included and no assets are staged. Stage explicit paths;
  do not use blanket staging that could include unrelated files or assets.

## Existing project workflow

Read `README.md`, `STATUS.md`, and `tools/README.md` for the current scope and
verification entry points. Keep routine evidence and coverage claims consistent
with the implementation and `docs/verified-routines.json`.

Build with `./build.ps1`. Run the configured regression suite with a local ROM
and asset pack:

```powershell
./build.ps1 -RomPath '<path-to-rom>' -AssetPack 'build/nba95_assets.pak' -Test
```
