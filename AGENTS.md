# Repository instructions

These instructions apply to all work in this repository.

## Manager and sub-agent responsibilities

- The manager assigns each sub-agent an exact native subroutine address range,
  its C implementation location, permitted changes, and required verification.
  Each sub-agent may implement only one assigned subroutine at a time.
  Independent subroutines may proceed concurrently in separate worktrees or
  explicitly disjoint files. The manager defines ownership and integration
  order; agents must not overwrite another agent's work or assume an unmerged
  prerequisite is already available.
- Sub-agents must use GPT-5.6 Sol (`gpt-5.6-sol`) with high reasoning. Do not
  substitute another model or reasoning level or delegate further.
- Sub-agents must read this file before working, stay within their assignment,
  and return the changed paths, behavioral findings, test commands/results, and
  unresolved limitations. Sub-agents must never stage, commit, or push changes.
- Only the manager reviews and stages changes, commits, and pushes. The manager
  verifies scope, test evidence, function comments, and the absence of assets
  before accepting an iteration and assigning that agent its next subroutine.

## Function comments and system documentation

- Every new or modified C function must have a comment immediately above its
  definition stating its native ROM address/range, system (for example gameplay,
  CPU logic, menu, rendering, or audio), and a short behavioral description.
  Preserve accurate existing comments. For a host-only helper, explicitly state
  that there is no direct native address and identify the routine it supports;
  never invent an address. Add missing comments as functions are touched rather
  than bundling a repository-wide cleanup into a subroutine iteration.
- Maintain a concise workflow document for each system being changed, such as
  `docs/cpu-logic.md`. Describe entry points, state inputs and outputs, dispatch
  and child-call flow, asset-pack dependencies, tests, and remaining gaps.
  Update it with the assigned routine's verified behavior, not speculation.
- Keep machine-readable routine progress in `docs/verified-routines.json` and
  regenerate affected coverage reports through the existing tools. Use the
  system workflow document for the next bounded routine and open questions;
  do not create conflicting completion ledgers or claim whole-game parity from
  isolated tests. System workflow documents are maintained documentation and
  are permitted alongside the existing status and generated evidence files.

## One subroutine per iteration

- Each sub-agent implements, ports, or fixes exactly one subroutine at a time.
  Identify its native ROM address or C function and the behavior being changed
  before editing. This limit applies per agent, not to the entire project.
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
- During implementation, use the focused routine and caller checks. Start the
  full suite after implementation and review have stabilized, so expected
  development failures do not repeatedly consume the long gameplay run.
  Repeat affected checks after runtime changes. For documentation-only changes
  or a focused assertion correction, retain already-passing evidence only when
  its executable, ROM, asset pack, and relevant test inputs are unchanged;
  rerun the changed assertion. Never treat stale or skipped results as passes.
- The manager integrates, tests, commits, and pushes each subroutine separately
  before assigning its agent another subroutine. Independent agents may continue
  their current assignments while another routine is reviewed or tested. If an
  iteration is blocked, resolve or report its blocker without bundling it with
  another routine. Recheck affected integration after combining worktree changes.
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
