# Gameplay verified-coverage plan: 40.08% to 45%

## Measurement and target

Recounted 2026-08-29 with `tools/progress.py` against every retained Mesen
`exec_*.txt` capture and `docs/verified-routines.json`.

| Metric | Captured address positions |
|---|---:|
| Executed denominator | 27,901 |
| Verified baseline | 11,183 (40.08%) |
| Minimum for 45% | 12,556 (45.00%) |
| Required verified gain | **1,373** |

These are captured address positions, not decoded-instruction counts, effort
points, or whole-game completion. Credit requires a bounded native entry/exit
contract, permanent ground-truth fixtures, a production C binding, and an
overlap-safe ledger range.

## Recount by remaining component/function family

The table intersects each range with captured execution and subtracts the
current verified ledger. Broad ranges are planning ceilings; nested helpers
must be recounted before credit so they cannot be added twice.

| Remaining component/function family | ROM range | Observed | Already verified | Pending ceiling |
|---|---|---:|---:|---:|
| Gameplay metasprite traversal, queueing, projection and OAM packing | `$80:9C75-$A7C5` | 1,523 | 0 | **1,523** |
| NMI gameplay OAM/queued-VRAM frame service | `$80:815A-$8626` | 659 | 0 | **659** |
| PPU palette and VRAM transfer helpers | `$80:8627-$8BF2` | 441 | 0 | **441** |
| Gameplay APU command/sample transport | `$80:A9B3-$AB05` | 211 | 0 | **211** |
| Compression/resource helpers (corrected by Ghidra) | `$80:C5AB-$CDCC` | 189 | 0 | **189** |
| Scene/resource helpers (corrected by Ghidra) | `$80:DA72-$E95A` | 194 | 0 | **194** |
| Controller serial scan and edge publication | `$80:CE33-$CE8D` | 78 | 0 | **78** |
| Bank `$84` gameplay data helper | `$84:BF75-$C014` | 59 | 0 | **59** |
| Remaining player animation/attachment tails | `$87:AAB2-$B952` | 1,254 | 1,205 | **49** |
| CPU inbound continuation (stale older table row) | `$86:F43A-$F668` | 213 | 213 | **0** |
| Player draw preparation (stale older table row) | `$87:A2CE-$A9CF` | 574 | 574 | **0** |

## Selected route

The initial `$80:A278` metasprite-parent capture crossed an NMI boundary, so
its broad WRAM delta was rejected rather than fitted. Clean child captures
remain useful future evidence: 140 allocator, 140 slot-selector, and 200 calls
each for axis, clamp and world projection, all with zero orphan exits.

The corrected selected route is the already production-bound hardware-visible
service path: `$80:815A-$8626` NMI/OAM/VRAM, `$80:8627-$8BF2` PPU transfer,
`$80:A9B3-$AB05` APU transport, and `$80:CE33-$CE8D` controller scan. Their
distinct pending ceiling is 1,389 positions. Host equivalents are accepted
only where raw native PPU/APU state or exact edge behavior is replayed; busy
wait and DMA latency are not treated as gameplay semantics.

## Implementation and verification phases

1. Dump and label the complete selected Bank `$80` listings, real entries,
   exits, nested calls and tables. Recount captured pending positions per
   boundary before translating anything.
2. Capture natural gameplay calls first. Add controlled cases only for native
   branch reachability; never patch an exit, ROM table, RNG result or expected
   output.
3. Translate/adopt one callable boundary at a time in reusable renderer/PPU
   helpers. Preserve object order, clipping, priority, palette, flip, OAM
   high-table and queue-overflow behavior. Runtime art remains asset-pack/ROM
   data—no emulator screenshots or frame captures.
4. For every checkpoint add permanent fixtures, a fixture-integrity/tamper
   guard, exact vector replay, production runtime binding, and an overlap-safe
   ledger entry. Re-run `tools/progress.py` after each commit.
5. Strengthen smoke/regression coverage with all-team player composition,
   Mode-1 pixel provenance, queue capacity/overflow, off-screen clipping,
   deterministic frame hashes and the 63,800-frame CPU endurance run.
6. Stop only at at least 12,556 verified positions. Run `build.ps1 -Test`,
   inspect every changed visual anchor, regenerate `docs/progress.md`, rebuild,
   recreate/read back the desktop shortcut, commit checkpoints and push clean
   `main`.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 11,183 (40.08%) | Generated ledger at goal start |
| Bank `$80` frame services | 1,389 | **12,572 (45.06%)** | Native PPU scanout parity, complete Mode-1 winner census, stamped APU/SPC replay, controller edge-state probe |

## Release result

The goal closed at **12,572 / 27,901 verified captured address positions
(45.06%)**, up from 11,183 (40.08%). The complete `build.ps1 -Test` gate
passes, including the new Bank `$80` host-service probe, exact frame-989 native
PPU replay (zero mismatches across 54,688 background and 182 goal OBJ pixels),
all 57,344 frame-1000 Mode-1 winners, original SPC700/S-DSP synthesis,
controller edge cases, every differential fixture, the 63,800-frame tip-flow
endurance run, CPU-versus-CPU gameplay hashes, and the license/legal/EA timing
suite.

The rebuilt `build/nba95_port.exe` and `build/nba95_assets.pak` are bound by
the recreated OneDrive Desktop shortcut to the F-drive ROM. The rejected
cross-NMI parent capture launcher was removed; only the clean callable-child
capture script and headless Ghidra dumps remain for future metasprite work.
