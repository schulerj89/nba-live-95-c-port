# Gameplay verified-coverage plan: 45.06% to 55%

## Measurement and target

Recounted 2026-08-29 with `tools/progress.py` against every retained Mesen
`exec_*.txt` capture and the overlap-merged `docs/verified-routines.json`.

| Metric | Captured address positions |
|---|---:|
| Executed denominator | 27,901 |
| Verified baseline | 12,572 (45.06%) |
| Minimum for 55% | 15,346 (55.00%) |
| Required distinct gain | **2,774** |

This remains captured-address coverage, not whole-game completion or a decoded
65816 instruction census. A range is eligible only when its native boundary is
understood, its portable production path is active, and a permanent behavioral
gate protects the native-visible result.

## Recount by component/function

The pending column intersects each planning range with observed execution and
subtracts the verified ledger. Broad rows are ceilings, not additive estimates;
nested ranges are recounted before any checkpoint is credited.

| Component / function family | ROM range | Pending ceiling | Verification focus |
|---|---|---:|---|
| Tilegroup/object projection and indexed draw services | `$80:8C00-$9C74` | **593** | Native tile/palette selection, transforms, window clipping and layer priority |
| Gameplay metasprite traversal, resource records, projection, clipping and OAM packing | `$80:9C75-$A7C5` | **1,523** | Native OAM order, flip, size, clipping and complete Mode-1 pixel provenance |
| Compression and ROM-to-WRAM/VRAM resource publication | `$80:C5AB-$CDCC` | **189** | Exact decompressed bytes, source identity, destination bounds and asset-pack integrity |
| Scene, fade, frame-wait and resource-dispatch services | `$80:CF00-$F100` | **923** | Exact title/setup fade cadence, snap/hold paths, PPU trace application and handoff frames |
| Bank `$84` gameplay data helper | `$84:BF75-$C014` | **59** | Table-boundary vectors before adoption |
| Bank `$85` camera/actor core | `$85:8000-$9FFF` | **347** | Differential actor/camera state, excluding verified child ranges |
| Bank `$85` ball/physics remainder | `$85:A000-$AFFF` | **56** | Ball state and collision outputs |
| Bank `$85` play/formation/defense remainder | `$85:B000-$CFFF` | **81** | Strategy/play stream state |
| Bank `$85` tail dispatchers | `$85:D000-$FFFF` | **489** | Bounded callers and alternate event paths |
| Bank `$86` shot/pass remainder | `$86:8000-$BFFF` | **451** | Owned launch/pass state only |
| Bank `$86` collision/ball remainder | `$86:C000-$DFFF` | **529** | Actor/ball/event ownership split |
| Bank `$86` AI/actor remainder | `$86:E000-$FFFF` | **636** | Alternate decision branches and callers |
| Bank `$87` gameplay draw/animation remainder | `$87:A000-$BFFF` | **244** | Resource/attachment results and queue order |

The older `gameplay-pending.md` inbound row is stale: `$86:F43A-$F668` is
already verified and contributes zero here.

## Selected route and checkpoints

The shortest evidence-complete route is the four independent Bank `$80`
families above. Their overlap-safe ceiling is 3,228 positions, leaving a
454-position margin above the 2,774-position requirement. This route is useful
to gameplay rather than percentage-only bookkeeping: it protects the live
player/ball/goal sprite queue, the ROM-derived graphics inputs it consumes, and
the frame cadence that publishes them.

1. **Tilegroup and metasprite pipeline.** Preserve the existing natural child captures for
   `$80:A34E`, `$80:A444`, `$80:A732`, `$80:A75E` and `$80:A781`; add a release
   probe covering raw-resource composition, negative/right-edge clipping,
   horizontal flip, OAM order/priority and live player composition. Cross-check
   the complete resulting frame against the retained native PPU oracle.
2. **Resource publication.** Re-run the ROM decompressor during asset tests,
   compare the exact gameplay VRAM/CGRAM and graphics-scratch payloads, protect
   every asset offset/size against overlap or truncation, and reject captured
   bitmap art as runtime input.
3. **Scene/timing services.** Lock both title-exit hold branches, the 15-level
   INIDISP fade, setup/team handoff cadence, trace rewind/forward identity and
   immutable frame hashes at the relevant transition anchors.
4. Recount after each ledger checkpoint. If overlap or an evidence failure
   leaves the result below 15,346, take only the smallest bounded gameplay
   caller from the remaining table needed to close the gap.
5. Run the entire `build.ps1 -Test` release gate, including all differential
   fixtures, all-team sprite composition, Mode-1 pixel parity, CPU-versus-CPU
   hashes and the 63,800-frame endurance run. Rebuild, recreate/read back the
   desktop shortcut, commit checkpoints and push clean `main`.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 12,572 (45.06%) | Generated ledger at goal start |
| Bank `$80` presentation/resource/timing services | 3,228 | **15,800 (56.63%)** | Natural child captures, 2,000-call raw sprite census, exact PPU bytes/pixels, asset directory/hash gate and frame-cadence regressions |

## Release result

The goal closed at **15,800 / 27,901 verified captured address positions
(56.63%)**, a distinct gain of 3,228 positions. The selected contribution is
593 tilegroup/object positions, 1,523 gameplay metasprite positions, 189
corrected compressed-resource positions and 923 scene/timing positions.

The full `build.ps1 -Test` release gate passes. In addition to every existing
native differential, it now requires:

- normal/mirrored raw gameplay resource composition and both clipping edges;
- 2,000 retained native raw-sprite calls across 25 resources and both flips;
- exact team-18 frame-989 VRAM/CGRAM identity plus all 29 packed court states;
- non-overlapping/bounded asset directory entries and immutable hashes for the
  animation, graphics-scratch, goal and indexed gameplay PPU payloads;
- deterministic title trace rewind, both ROM hold paths and all 15 fade levels;
- exact Mode-1 background/goal pixel parity, all-team sprite composition,
  CPU-versus-CPU hashes and the 63,800-frame endurance run.

No screenshots, Mesen captures or flattened emulator frames are used as live
player/goal art. The runtime continues to use ROM-derived asset-pack resources.
