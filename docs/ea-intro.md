# EA intro reconstruction

**2026-08-30 audit correction:** the historical path below was incomplete and
some provenance claims were wrong. Its PNG/handwritten intro graphics have now
been replaced by indexed pack75/76, and its selected-frame/mean-error gate by
an exact303-frame renderer comparison. See
[intro-indexed-resources.md](intro-indexed-resources.md) and the independent
audit linked there. The following is retained as historical implementation
context. Full-intro timing, input, audio and handoff remain unverified or wrong.

The EA intro is reconstructed from ROM/PPU state rather than from emulator
screenshots. Asset-pack version 18 contains independent indexed E, A, and
SPORTS layers captured from VRAM with their ROM palettes.

## SPORTS path

- `$82:F52E` calls the shared tilegroup renderer `$80:8FA3` twice with the ROM
  descriptor at `$82:F6D8`, placing its halves at tile rows `$38` and `$3D`.
- `$82:F56D` runs the Mode 7 entrance. SPORTS has a preparation frame before
  its matrix changes, producing `1,1,1,13,25...` rather than E/A's
  `1,1,13,25...` sequence. The register callback sees an update during the
  frame; the completed picture presents it one frame later. The C renderer
  preserves that presentation delay so motion 73 is still offscreen and
  motion 74 contains the first visible SPORTS slice.
- `$82:F4C4` calls palette-step routine `$82:F64A` twice per wait. The observed
  highlight is `base,+6,+12,+18,+24,+30,base,+6,+12` BGR555 levels.
- Stage 3 carries forward the final fixed-OAM EA frame produced by the preceding
  `$82:F4C4` path. `$82:F408/$82:F52E` build SPORTS but do not reconstruct EA;
  using the older pre-OAM Stage 2 screenshot causes a visible EA flicker just
  before SPORTS appears.
- E remains on its raw `$82:F4F6` Mode 7 layer through identity and both flash
  sweeps. The ROM does not replace it with the older completed-stage bitmap;
  doing so moved the settled E four pixels left and roughly four pixels up.
  Stage 2 likewise draws this same identity E while `$82:F512` introduces A.
- The SNES Mode 7 transform uses the same low-six-bit truncation as the verified
  recompilation PPU before its final 8.8 fixed-point shift.

`tools/mesen_intro_capture.lua` saves `ea_sports_mode7_vram.bin` immediately
after both tilegroup draws and saves the committed CGRAM palette during the
entrance. `tools/extract_assets.py` decodes visible palette indices `$21-$2F`
into asset `NBA_ASSET_EA_SPORTS_LAYER`; `$11-$1F` are black clearing cells, not
visible artwork. This avoids the old shortcut that subtracted two flattened
screenshots and accidentally included ghost E/A pixels.

## Regression proof

`tools/test_intro_sequence.py` verifies pack version 18 and the presence and
dimensions of the raw SPORTS layer. It locks early/late zoom frames, all nine
palette-highlight frames, and compares them with address-synchronized Mesen
oracles. A source guard also rejects restoration of the screenshot-difference
path.
