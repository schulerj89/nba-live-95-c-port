# Captured-address verification plan: 85.43% to 100%

## Measurement and target

Recounted 2026-08-29 with `tools/progress.py` against all retained Mesen
`exec_*.txt` captures and the overlap-merged verified ledger.

| Metric | Captured address positions |
|---|---:|
| Executed denominator | 27,901 |
| Verified baseline | 23,837 (85.43%) |
| Exact 100% target | 27,901 (100.00%) |
| Required distinct gain | **4,064** |

This is exact coverage of the retained native execution captures, not a claim
that every ROM instruction or every game mode has been reconstructed. Captures
from gameplay also execute shared scene, menu, text, audio and hardware-service
code, so reaching 100% requires verifying those shared families too. No 99.x%
value rounds up.

## Complete zero-remainder inventory by component/function

The pending column intersects each disjoint planning range with observed
execution and subtracts the verified ledger.

| Component / function family | ROM range | Pending positions |
|---|---|---:|
| Bank `$00` bootstrap/native vector prefix | `$00:8000-$FFFF` | 30 |
| Bank `$80` bootstrap entry | `$80:8000-$8159` | 1 |
| Bank `$80` PPU transfer-helper tail | `$80:8BF3-$8BFD` | 5 |
| Bank `$80` late object/text helpers | `$80:BF01-$BFFF` | 97 |
| Bank `$80` interpreter/dispatch micro-entry table | `$80:C000-$C467` | 394 |
| Bank `$80` resource-publication prelude | `$80:C468-$C5AA` | 166 |
| Bank `$80` controller-publication tail | `$80:CE8E-$CEFD` | 3 |
| Bank `$80` final scene/timing tail | `$80:F101-$FFFF` | 77 |
| Bank `$81` text/font/object/menu prefix | `$81:8000-$AFFF` | 1,339 |
| Bank `$81` Player Setup/controller scene | `$81:B000-$BFFF` | 344 |
| Bank `$81` transition/scene services | `$81:C000-$CFFF` | 266 |
| Bank `$81` Set Rules menu | `$81:D000-$DFFF` | 475 |
| Bank `$81` presentation/HDMA tail | `$81:E000-$FFFF` | 165 |
| Bank `$82` scene tail D | `$82:D000-$DFFF` | 9 |
| Bank `$82` scene tail E | `$82:E000-$EFFF` | 11 |
| Bank `$82` EA/gameplay-scratch/audio tail | `$82:F000-$FFFF` | 476 |
| Bank `$83` presentation prefix | `$83:8000-$BFFF` | 11 |
| Bank `$84` gameplay table/helper prefix | `$84:8000-$BFFF` | 50 |
| Bank `$84` animation-table dispatch | `$84:C000-$DFFF` | 9 |
| Bank `$84` roster/graphics data helpers | `$84:E000-$FFFF` | 136 |
| **Exact remaining total** |  | **4,064** |

Banks `$85`, `$86`, and the captured portion of `$87` already have zero
remaining positions.

## Zero-remainder execution plan

1. Run a fresh all-capture headless-Ghidra closure for Banks `$00/$80-$84`.
   Retain decoded listings and static call maps for every observed family.
2. Cross-check Banks `$00/$80-$82` against every recomp root and its native/LLE
   fallback boundary. Banks `$83/$84` remain Ghidra/native-owned because the
   current recomp does not statically discover them.
3. Build a machine-readable closure audit that enumerates all 4,064 baseline
   positions and refuses release if any position is absent from the final
   verified ledger.
4. Add an aggregate production journey gate covering legal/EA/title, Game
   Setup, Rules/Options persistence, Team Select, Player Setup, matchup,
   lineups and gameplay handoff. Hash state and rendered output at each native
   ownership boundary while retaining the existing exact per-function and
   per-scene oracles as primary evidence.
5. Implement or correct any behavior exposed by the audit. If a position is
   transient SNES glue, verify its final native-visible state rather than
   pretending DMA latency or interrupt cycles are portable gameplay logic.
6. Recount only when every family has permanent Ghidra/recomp provenance,
   production binding and regression evidence. Require exactly 27,901 verified
   positions and a zero-item remainder report.
7. Run the complete `build.ps1 -Test` suite, rebuild, recreate/read back the
   desktop shortcut, commit and push clean `main`.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 23,837 (85.43%) | Generated ledger at goal start |
| Bank `$00` bootstrap/vector closure | 30 | 23,867 (85.54%) | Fresh Ghidra 19-start decode; host process/input publication boundary |
| Bank `$80` shared-service closure | 743 | 24,610 (88.20%) | Ghidra/recomp audit; PPU/OAM/audio/resource/input/scene gates |
| Bank `$81` menu/presentation closure | 2,589 | 27,199 (97.48%) | Transition traces plus Rules/Options and Player Setup production journey |
| Bank `$82` scene/resource closure | 496 | 27,695 (99.26%) | EA, scratch, SPC/DSP and Team Select exact gates plus journey persistence |
| Bank `$83` RNG/event closure | 11 | 27,706 (99.30%) | Exact RNG/event witnesses and seeded live journey |
| Bank `$84` table/resource closure | 195 | **27,901 (100.00%)** | Raw table/animation/appearance replays and live resource census |

## Closure audit result

- Fresh headless Ghidra closure retained decoded listings and call maps for
  Banks `$00/$80-$84` under `.analysis/gameplay100-closure-ghidra`.
- `tools/verify_captured_closure.py --expect-pending 0` reports exactly 27,901
  executed, 27,901 verified, zero pending and zero unaccounted positions.
- `tools/progress.py` independently reports both documented provenance and
  verified ground-truth coverage at 27,901/27,901.
- `gameplay100_closure_probe` runs the production APIs twice and pins digest
  `39974258c482a822`: eight scene/page handoffs, 65 changing render samples,
  2,910 actor-motion frames, 12,985 animation-resource changes and 64
  possession changes.

The percentage is strictly the retained captured-address metric. It does not
claim that unobserved ROM code, unsupported modes, or every full-game feature
is complete.
