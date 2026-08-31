# Paused checkpoint — 2026-08-30

Work stopped at the user's request. **The complete game is not finished.**
Main retains the independently reviewed A1/A2 runtime; unfinished work is
committed and pushed separately so it does not replace that desktop build.
No background implementation, capture or monitoring job remains scheduled.

## Pushed work

| Checkpoint | Commit | Scope |
| --- | --- | --- |
| A1 | `e1bc0d4db83c1e19998e025cf653650aedc62437` | Rules opening/hold/return fixes, correct brightness, new-match reset, stronger evidence and verifier checks |
| A2 | `0d3a4207805aa7bea307cc1709e1f808acf7e54b` | ROM-derived indexed intro packs, integer EA renderer, original font/text renderer and exact native image checks |
| Saved WIP | `c134f85de7e88b4ffd9017908322a84c295b0e42` | Configuration/resource/team-identity integration plus frozen HUD/audio/scheduler work; not merged into main |

Saved branch: `work/ownership-stop-20260830`. Its detailed report is
[`docs/ownership-stop-wip.md`](https://github.com/schulerj89/nba-live-95-c-port/blob/c134f85de7e88b4ffd9017908322a84c295b0e42/docs/ownership-stop-wip.md).
All source, tests and documentation from the frozen workstreams are preserved
there. Original ROM, captures, extracted packs, audio recordings, compiled
outputs and bulky traces remain ignored locally.

## What works in the main/desktop checkpoint

- The configured first Rules opening matches147 consecutive original frames;
  the reveal/hold matches208 consecutive frames; unchanged and Custom returns
  match342 consecutive frames. These are the documented configurations and
  frame mappings, not every menu transition or repeated visit.
- A normal new Exhibition resets the previous match's final/period/score/
  timeout/lineup state while retaining session settings. This fixes the
  second-match freeze in the tested normal return journeys.
- EA's corrected integer renderer matches303 consecutive original animation
  frames. License/legal text uses the original font, strings and palettes;
  five production samples and31 brightness rasters pass their bounded gates.
- The new intro graphics come from ROM-derived indexed asset-pack resources.
  Screenshot-based intro entries and the handwritten license fallback were
  removed. This does not certify every older graphic/audio path in the game.
- The complete A2 regression test list passed, including deterministic CPU,
  lifecycle and multi-team endurance checks. Those C checks are kept distinct
  from independent original-ROM comparisons. See `ownership-checkpoint-a2.md`
  for the exact logs, denominators and fixture corrections.

The audit also corrected misleading evidence methods: asynchronous emulator
screenshots were replaced with synchronous native buffers; the lifecycle
verifier now consumes its expected values; inbound comparisons capture missing
internal same-dispatch state; and C golden hashes are labeled as regressions,
not ROM equivalence. Independent auditors inspected source and evidence.

## What is saved only as WIP

- Correct factory defaults, working/committed menu values, Custom Rules and
  native button-repeat behavior. The integrated bounded gates pass730
  configuration checkpoints,1,770 adjustment observations and47 canvases.
- Correct home/visitor team identity, appearance/rating consumers and sorted
  actor ranks. Root's integrated initializer probes match128 native words
  across two team selections; full initialization is not claimed.
- Rules DMA publication fixes, including explicit zero writes. Repeated-entry
  timing remains failing; fresh native evidence identifies the actual NMI
  epoch-wait contract, with no invented delay applied.
- HUD child publishers/clock formatters and isolated audio-event translation.
  Their bounded native probes pass, but normal callers, shared RNG/NMI timing,
  final HUD visibility and audible sequence parity remain unfinished.

The combined WIP build compiles, but its old headless menu driver still sends
pressed-only input instead of held/released input required by the new native
producer. Root reproduced the failure. Its full suite and combined independent
audit are incomplete, so it was intentionally not merged or installed.

## Remaining game acceptance failures

Ordinary human controls/ownership, complete CPU-versus-human and CPU-versus-CPU
native journeys, all runtime rules/options consumers, all transitions,
complete lifecycle presentation, dynamic HUD, native audio scheduling,
Season/Playoffs/load/persistence, and full synchronized differential acceptance
remain incomplete or materially unverified. Original intro hold/input/audio
timing also remains incomplete despite the bounded renderer matches.

Instruction coverage counts annotated instruction positions in an observed
corpus; it excludes unobserved code and says nothing by itself about caller
wiring, runtime reachability, image correctness or whole-game completion.
No completion percentage is endorsed by this handoff.

## Desktop and continuation

The shortcut `NBA Live '95 (C Port).lnk` points to `build/nba95_port.exe` with
the original ROM and `build/nba95_assets.pak`. The separate Recompiled shortcut
was not changed. At stopping, the executable and pack still match the tested
A2 identities:

- EXE SHA256: `dc29aec6fd1ee81f30e899f29984dd6f3e51a36c2edd50916247efc8b7179651`.
- Pack SHA256: `c7b90d9347c257e0746da7a6d5595e603ffd9d3a026666fe6e62c4f483e75a92`.

A final replay against that desktop executable again passed303 native EA
frames and five text samples (`build/ownership-stop-desktop-validation.log`).
The repository/reference map and detailed remaining inventories are in
`ownership-plan.md`, `transition-ownership-audit.md`,
`gameplay-ownership-audit.md`, `options-test-ownership-audit.md` and
`asset-ownership-audit.md`. Consult the WIP report before resuming integration.
