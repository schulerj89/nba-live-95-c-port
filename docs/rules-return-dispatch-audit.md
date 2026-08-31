# Rules dispatch-frame independent audit

**PASS for the first-return dispatch-frame repair and the stated RGB/PPU
scope. Whole-resource timing and repeated entry remain unverified or failing.**

The previous first-return gate began at native831/C528. It omitted the Start
dispatch frame830/C527. An independent replay of the retained executable
`35c490b9941d1f6d15bb9dc2d245143a180da8a4cad4e4f47838d2a78bfa69f9`
reproduced6,464 wrong pixels for the unchanged Rules menu and6,498 for the
row2/OFF/Custom case, within x19..191,y23..202. C displayed a nearly empty
Rules panel and published end-frame main-screen designation23 instead of3.
All170 subsequent frames still passed. Therefore the old170-frame PASS was
insufficient evidence for a clean complete return boundary.

## Independent reproduction and native evidence

Evidence lives in `.analysis/rules-return-t0-audit-20260830/`:

- `old-v2-report.json`, `old-v2-hold/`, `old-v2-custom/`: reproduced failure,
  complete171-frame C output, exact launch commands and PPU telemetry.
- `native-hold/`, `native-custom/`: fresh original-ROM runs in separate
  portable Mesen directories, empty initial save folders, fixed video/filter
  settings and disabled frame skipping. Ordinary controller input selects
  Simulation/three-minute quarters and traverses the real menus. No CPU,
  ROM, WRAM, SRAM or PPU state injection is used. Each run exits0.
- `after-fresh-report.json`, `after-fresh-hold/`, `after-fresh-custom/`:
  independent direct byte/pixel and scalar-state comparison against those
  fresh captures, separate from the implementer's fixture checker.
- `fresh-completion-report.json`: all548 raw RGB files from each fresh run
  match its earlier native journey exactly; the complete opening and return
  PPU traces also remain identical. The added dispatch telemetry is read-only.
  Frozen-script differences and original-ROM bytes are retained in this report.

Fresh native manifest SHA-256 values are:

| Journey | SHA-256 |
|---|---|
| Hold | `a6a7f3d22ec1ef2e133581c1a432ded112a37867d2b4b1d004ea65a82756ef7e` |
| Row2/OFF/Custom | `97cd9aeefffc7ebbfda813ac2b9008bdfcac5f19903dfc7506e9009b29a2348b` |

The new `dispatch_ppu_states.txt` records complete PPU states at native470
(A-confirm) and830 (Start-confirm). Its142 bytes have SHA-256
`9fb1fab80ab24b29e3cf660ca2170be9af0746f34d5136c6af41a5b0a4f00dbc`
in both runs. No state is inferred from the following frame. In particular,
native830 has BG2v93 and native831 has BG2v94. Native830's brightness/main/sub
are15/3/4. Both dispatch records are attested in the capture manifest.

The authoritative RGB source is synchronous `emu.getScreenBuffer()` at
endFrame, with a fixed256×239 buffer and visible rows7..230. The PNGs created
by `emu.takeScreenshot()` are asynchronous evidence: the native830 PNG still
shows the previous presented highlight although the synchronous830 RGB has
already removed it. They must not be cited as exact same-frame pixels. The
auditor converted the raw RGB to `custom-830-synchronous-rgb.png` and viewed it
beside `after-fresh-custom/frame_0527.bmp`; both retain the live Rules header,
OFF value and sliders, with the selection highlight correctly disabled.

## Owning behavior and the repair

The auditor inspected the actual USA ROM, the independently regenerated
`reference/bank81-with-repeat.c`, and Ghidra
`ghidra/with-repeat/setup_config_bank81.txt` under
`.analysis/setup-config-native-20260830/`.

`$81:D54A-$D577` clears `$2125`, calls `$81:A975/$81:A981`, then waits through
`$80:86B0` before clearing the menu job `$174B`, clearing `$2125` again and
invoking the transition services and `$81:C440`. The actual46 ROM bytes are:

```text
e220a9008d2521c2202275a9812281a98122b086809c4b17e220a9008f252100c22022688680222786802240c481
```

The fresh native raster writes retain the active viewport during830:
`$212C=07` at scanline2,17 at79, and03 at203. Teardown writes occur on the
following dispatch. End-frame designation03 therefore cannot alone describe
the just-completed image. This is why selecting the prepared return canvas
as soon as Start was consumed removed the still-visible Rules content.

The repaired C renderer keeps the live Rules canvas, working values, cursor,
viewport and object scanout on transition frame0, while disabling the color
window and publishing native end-frame designation03. It switches to the
return trace on the next update. Its local render cursor reset is deferred
until that trace update instead of happening before the outgoing image.
No production delay, invented fade, crop or extra blank frame was added.
The exact timing of hidden native row-memory changes is outside this PPU/RGB
projection; deferring the local render cursor does not prove those bytes.

The BG2 publisher is separately confirmed at `$87:89D5-$89E8`, called from
`$81:F9FC`: it advances `$168F` modulo three and increments `$0613` on rollover.
The original bytes are `ad8f16c90200d0089c8f16ee13068003ee8f166b`.
`$174B` is a menu/OAM job request, not the BG2 counter. This audit covers the
observed phase-crossing return; the other entry phases are not inferred.

## Exact accepted scope

Frozen repaired executable SHA-256:
`bb9c2114ac110ff6f36134a615e7a898b91acdd689f72084a204171aa34b4082`.
Frozen `nba_setup_screen.c` SHA-256:
`a3fd53c8360a7de7b3f19b5bc5dd6571de3ed40b5e1ee5e46a1f8bdb6f1428f6`.
Pack SHA-256:
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.

| Comparison | Result |
|---|---|
| Two first returns, native830..1000/C527..697 |342 consecutive RGB frames;19,611,648 pixels exact |
| Native830..962/C527..659 |266 complete states;5,852 PPU scalar fields exact |
| Opening including A-confirm, native470..616/C884..1030 |147 RGB frames and147 full PPU states exact against each fresh corpus |
| Fresh versus earlier native evidence |548 raw RGB files per journey and both full PPU traces identical |

The return C input waits remain212 idle frames for the unchanged menu and209
after three row/value presses for Custom. Opening uses the previously declared
717 input-idle alignment. These are disclosed harness alignments; earlier C
and native navigation timelines are not asserted identical. Comparisons use
no tolerance, timing search or output-selected frame offset.

The durable return gate now includes830/C527 and requires the independently
attested dispatch PPU file. The opening gate also includes470/C884. The auditor
reran both171-frame return fixtures, the147-frame opening fixture and all27
transition-integrity tests successfully after those gate updates.
These observations do not resolve the known intermediate Custom VRAM/upload
divergence or repeated-entry failures documented in
`docs/transition-independent-audit.md`. They do not prove production asset
provenance, full native routine execution, configuration defaults/presets,
other transitions, gameplay or audio.
