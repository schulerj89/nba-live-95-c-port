# Pending gameplay: first differential-harness trial

2026-08-28. This starts the user's requested table; it does **not** complete
the table or claim full-game behavioral equivalence.

## Corrected census and progress

| Requested area | Corrected starting count | Verified this increment | Still pending |
|---|---:|---:|---:|
| Jump/reach startup and continuation | 239 | 0 | 239 |
| Ball initialization prefix | 30 | 30 | 0 |
| Defensive idle/pose decisions | 99 | 0 | 99 |
| CPU inbound continuation | 224 | 0 | 224 |
| Timeout confirmation prefix | 9 | 0 | 9 |
| **Bounded core** | **601** | **30** | **571** |
| Optional human inbound steering | 19 | 0 | 19 |

The former31-instruction ball count was incorrect: bytes at `$86:E053` are
`4C A7 DD` (JMP DDA7). `$E054` is an operand, not an executable LDA. Native
execution falls through the preceding BEQ to `$E056`. The corrected prefix
is `$86:E056-$E0AB`,30 starts. This changes602 to601 before subtracting any
implemented work; it is not implementation credit. The other rows recount
unchanged in `.analysis/pending-diff-ghidra-20260828/tip_flow_bank86.txt`.

The new capture initially failed at E054 instead of inventing a witness. Both
the older tip-off Ghidra seed and F8/CLI routine constant are now corrected.
`DumpTipoffFlow.java` labels/comments the actual E056 prefix.

## What was implemented and verified

`nba_tip_ball_initialize` translates the prefix's20 WRAM word outputs into a
small C state object. `nba_tipoff_init` now uses it for initial position,
upward velocity600, published ball pointer0910,4933/4935 and08F0 sentinels.
The native object-list link at34E7 is represented as bookkeeping data, not
a host pointer or emulator-rendered asset. `$86:D5E1` initializes list base
34D3; ten two-byte actor links precede the ball entry.

The old C initializer left VZ zero and08F0 zero. Native E092 writes0258 to
ball+$12, and E0A3 writesFFFF to08F0. The Gameplay Lab regression assertion
now expects that exact native sentinel; it was not weakened to accept either
value. The pending-foul event and whistle flags remain zero.

Proof uses three distinct sources:

- Fresh Ghidra contiguous decoding from the ROM, and regenerated bounded
  snesrecomp C at `.analysis/pending-diff-reference-20260828/bank86.c`.
  `tools/regenerate_pending_reference.py` regenerates references for all table
  areas without modifying the user's reference project. These bounded
  fragments have unresolved external continuations; they are not executed
  as an additional oracle or linked into the port.
- One natural Mesen E056/E0AC entry/exit and one controlled native call with
  nonzero values in all19 output words other than the valid34E7 cursor.
  Each executes all30 starts. No ROM, PC, CPU flags or stack changes are used.
- The production C helper receives the identical captured entry inputs through
  a test adapter. All20 projected words AND the entire128-KiB exit snapshot
  match, with zero mismatches. Unrepresented bytes must remain unchanged.
  A separate runtime test verifies the real game initializer consumes these
  outputs rather than leaving a correct but unused helper.

Native entry ABI is explicitly checked: D=0, DBR=7E, M/X=16-bit. The pointer
contract is the ordinary ten-player startup cursor34E7; arbitrary aliasing
RAM pointers and cycle/register equivalence are not claimed. This is a
same-entry bounded differential replay, **not** a full-game RAM importer.

Durable fixtures: `tests/fixtures/ball-initialization.json` and
`ball-initialization-poisoned.json`. Their outputs are native, not C-generated.
Fixture replay additionally fills unrepresented RAM with a deterministic
nonzero pattern to guard preservation. Fresh captures remain in ignored
`.analysis/pending-diff-20260828-final` and `...-poison` (earlier after-run
evidence is also retained), including
`ball-init-report.json`, snapshots, native PCs, addressing metadata and hashes.

The ordinary442-word game trace improved from82 to81 baseline differences
under the preserved CPU-versus-human reference setup. The only removed
mismatch is `ball.vz`; none were added. It still returns
`INITIAL_STATE_MISMATCH`, not a PASS. Initial RNG, other setup state,
controller implementation and real sweep scheduling remain different.

## Commands and regression protection

```powershell
./build.ps1 -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
./tools/build_vector_probe.ps1 -Name differential_runtime_probe
./tools/build_vector_probe.ps1 -Name ball_init_differential_probe
python tools/run_differential.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --pack build/nba95_assets.pak --output .analysis/new-prefix-trial --capture-ball-init --sweeps 2
python tools/verify_ball_init_differential.py --capture .analysis/new-prefix-trial --probe build/ball_init_differential_probe.exe --report .analysis/new-prefix-trial/ball-init-report.json
```

The first command that runs Mesen currently exits1 for the separately reported
whole-game baseline mismatch; valid prefix captures are still produced. Add
`--poison-ball-init` to the runner for the controlled native test, always with
a new output directory. CPU-only remains optional, not the reference default.

`build.ps1 -Test` includes both native fixtures and the runtime binding check.
`test_differential.py` additionally tests explicit entry/exit plans so slice
replay does not fabricate actor-sweep checkpoints. Tip-off, Gameplay Lab,
observer non-mutation and the63,800-frame CPU endurance checks pass after the
initialization change. Full release status is recorded separately below.

Release verification: `build.ps1 -Test` completed with exit0; log
`.analysis/pending-diff-20260828-release.log`. This includes native replay,
menus/assets/audio, player labs, tip flow, long CPU simulations and intro tests.
A subsequent rebuild includes the corrected F8/CLI E056 address, followed by
the Gameplay Lab regression, initializer binding and fresh native prefix test.
The working changes have not been committed or pushed in this increment.

## Next work, not hidden by this result

Jump/reach update later on2026-08-28: the239-start parent is now translated
and tested in `src/nba_jump_reach.c`; see `jump-reach-differential.md`.
651 native calls match the decision projection, including100 animation
starts matched through the existing channel API. Normal-game adoption has
not occurred, so the table's239 pending and aggregate571 remain unchanged.
The list below describes the earlier checkpoint; its first item is now
partly completed (capture, decision translation and asset mapping).

1. Jump/reach239: capture EC32/ECF9 branches with native entry state, caller
   order and actual upper/lower animation channel commands. ED3D/EDFA read
   player attributes through E0; ED5F consumes the EE76 threshold table. Those
   dependencies need proved asset-pack mappings, not guessed values.
2. Replace the current pre-contact quadratic ball/jump presentation and align
   the initial dispatcher. It currently overwrites the correctly initialized
   VZ on its next update. Therefore this prefix correction alone does NOT fix
   the visible toss, jump timing, sliding or camera trajectories.
3. Defensive99, CPU inbound224 and timeout9 remain separate subsequent slices.
   The optional human19 branch is not implied by preserving a human assignment
   in the ROM reference; the port's human controls are still unimplemented.

Verified captured-address coverage is now7,615/27,901 =27.29% (152 slices),
up30 address positions from27.19%. This is not a percentage of the whole game
or proof that the remaining callers behave equivalently.
