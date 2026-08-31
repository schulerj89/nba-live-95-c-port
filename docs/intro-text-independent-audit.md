# Independent indexed intro and native font audit

Auditor: gameplay workstream, 2026-08-30. This follows the earlier
`ea-indexed-independent-audit.md` review. The implementer did not supply the
auditor's C outputs or expected hashes.

**PASS for the bounded resource, font raster, and EA motion scope below.
FAIL/open for complete intro execution, timing, input, audio and title handoff.**
No full-game or all-asset provenance claim follows from this checkpoint.

## Independent replay and denominator

The auditor freshly compiled the intro worktree's final integer-motion API
and text/resource probes into
`.analysis/ea-independent-audit-20260830/font-recheck/`. The complete game
executable SHA-256 is
`360c5038748500be0aa2cfa218284f50b5282bc8f8a5fd23243d0774dbda21d8`.
The reviewed candidate pack SHA-256 is
`c162e9ebe5287c9f5c46e24904449d29e45238853c4b515ce4734aaa44fe458a`.

The following independent comparisons passed without pixel tolerances:

- 303 consecutive EA motion images, native motion0..302 / license-relative
  frames540..842, mapped explicitly to C frames345..647. Each comparison
  covers all57344 pixels of the native256x224 viewport. Late images include
  a static hold:303 is a frame count, not303 distinct behaviors.
- Five C text-phase samples against independent native RGB: full/dim license,
  rising/full/dim legal. These are selected renderer states, not a continuous
  cold-boot journey.
- 31 text rasters at matching native brightness: license1..15 and legal0..15.
  One additional C license brightness0 output matches the native forced-blank
  black reference. Native frame143 actually has brightness15 and forced blank;
  it is explicitly **not** proof of license execution at native brightness0.

The303-frame mapping preserves the known C345-frame lead-in only to select
the same EA routine phase. Native EA begins at license-relative540. This
offset is not accepted as equivalent initialization or native scene timing.

Reports:
`font-recheck/intro-sequence-final.json`, `text-final.json`, and
`independent-resource-report.json`. Native source is
`.analysis/intro-exact-20260830/capture-v4`, manifest SHA-256
`8121eebf0e84bec702ba351121f3df7460471a0e513aecda5ba67a7d394071eb`.
The auditor checked all1807 attested artifacts for exact size and SHA-256,
the executed Lua script, private Mesen executable/settings and observed Lua
home. Native RGB comes from synchronous `getScreenBuffer`; rows7..230 are
the fixed224-line viewport specified by the capture settings. This was a
natural no-input cold boot, not a save-state or WRAM-injected run.

## Source and production resource findings

Read the original ROM, independently verified the bank80/81/82 reference
binaries byte-for-byte, then inspected Ghidra and generated recomp owners:
`$80:FDC2-FDF1`, `$FE38-FE58`, `$FEAD-FED8`, `$81:9756-9FFD`, `$9B09-9BC4`,
`$9F54-9FA2`, `$A163-A1A6`, and the earlier audited EA routines.

The portable font retains the native centering width, original-case lookup,
lowercase-to-uppercase glyph mapping, the two-row capital/digit expansion,
the expansion-table row numbering, proportional glyph width and glyph OR.
Native18CE adds the two-pixel baseline for unexpanded glyphs; legal18D0 also
expands digits. The legal caller begins at y40, advances descriptor height+3
on CR and restores its x origin; license starts at y104. Full legal output,
including the copyright glyph, was visually inspected. This is a bounded
2bpp font translation; other native font formats are not declared complete.

The auditor instrumented both resource builders' file reads. Neither read
RGB, PNG, BMP, WAV or other rendered production artwork. Rebuilt payloads:

| Asset | Bytes | SHA-256 |
|---|---:|---|
|75 indexed EA|71674|`90800623cd1734cfb41523cb3c89a428d542cb44dfefb12c9c4a35fdf3a3a514`|
|76 intro text|4568|`0b5ba5640fde0bc6680ce3e8dcd29381e98b63a951916f3404568ebd4c53dd6d`|

Asset76's4096 font bytes,172 expansion-table bytes and252 string bytes were
checked directly against the pinned ROM. Its eight palette bytes are native
ROM-produced CGRAM from the documented compressed-palette owner; independent
format30 decompression remains pending. Asset75 retains the earlier exact
11776-byte direct decompression PREFIX comparison for the Mode7 character plane. Its
static format30 base map/OBJ resources likewise remain native-resource
extractions rather than independently decoded compressed streams.

Compared with the accepted pre-checkpoint production pack, the candidate
removes IDs1..6 and70..74, adds75/76, and leaves every common entry's metadata
and bytes unchanged. C initialization now requires both new resources and
does not silently fall back to a bitmap license/font or captured EA image.
This audit directly rebuilt the two resources and inspected candidate entries;
the root integrator separately owns a complete normal-extractor reproduction.

## Reusable evidence gate review

The earlier duplicate-JSON, trusted-success-flag, partial-header and missing
launch provenance issues are fixed. The builder checks the pinned reviewed
script, original ROM/Mesen identities, successful completion, empty initial
saves, initial/current settings, actual executable, observed Lua home and
every consumed artifact's exact length/hash. Resource loaders validate full
headers, dimensions, metadata, lengths and text termination.

Freshly reran the implementer's eight resource-integrity tests, covering116
rejected mutations, and independently checked12 additional malformed
provenance cases. All passed. These are harness-integrity tests, not native
gameplay evidence.

The auditor's permanent `tools/test_intro_frame_provenance.py` adds seven
test groups for missing/duplicate/reordered1500-frame and11-mark populations,
every relevant field's type/presence, duplicate JSON keys, frame indices,
PPU value bounds, EA entry identity, selected text/EA phase metadata,
image geometry and failure on every differing pixel. Its synthetic black
rasters are explicitly comparator-integrity fixtures, never native parity
fixtures. All seven groups passed against the final source.

The final sequence gate requires indexed assets, rejects the legacy IDs,
checks original ROM identity, uses a bounded subprocess timeout and validates
attested frame/entry-state metadata before reading expected native images.
No golden hash was replaced by C output and no MAE envelope remains here.

## Remaining exclusions

- No accepted exact cold-boot scheduler/resource-upload timing, reset/skip
  behavior, complete EA hold, EA-to-title transition or whole intro journey.
- No accepted native intro audio command/sample/sequence timing. Existing
  assembled/offline audio behavior is outside this graphics checkpoint.
- No independent format30 decoder proof for the memory-extracted portions
  above. Source bytes are ROM-produced resources, not screenshot-derived art.
- No claim that every font character/style/format or every original text
  caller is translated; the tested callers and font modes are stated above.
- No claim that all other assets, screens, game modes or gameplay are complete.

The accepted result is a portable resource/font rendering checkpoint with
specific independent native images and strict evidence handling. The open
timing and behavioral work must not be hidden by the phase-aligned PASS.

## Tracked runner and debugger metadata addendum

Independent review of `tools/capture_intro_indexed.py` confirms pinned ROM,
Mesen and immutable Lua identities, a new private portable home, a per-child
clean environment, no supplied input, a 200-second subprocess timeout, and
validation of completion and raw resource provenance before acceptance. The
tracked Lua SHA256 remains
`d8bb668ccffbbe6346b9d6720be0236820a40135c217a13c5e020569f9f1f1d7`.
Compared `capture-v4` with `tracked-runner-v1` directly: 1,805 common files
are byte-identical, including the script; the two different common files are
private path/settings observations. The implementer's 1,804 comparison includes the copied script and excludes
`mesen.log`; the independent count additionally includes that identical log. These
counts do not establish additional gameplay or intro timing coverage.

The six F12 expected C hashes may change for the new asset directory. I
independently reran all 12 retained-old/current full/reduced debugger cases
for asset IDs126,128,160. All old and new hashes reproduced. `nba_asset_debugger.c`
prints directory index/count at x8,y19; the production directory changed
272 to263 entries, reduced directory266 to257. Direct RGB comparisons show
81-104 changed pixels per case, all within x8..62,y19..25. Every pixel outside
that metadata is identical. I viewed the old/current ID126 pair as well.
This is **PASS for C debugger regression migration**, not native game proof.
No asset pixel was changed to make the gate pass. Evidence is
`.analysis/rules-resource-independent-20260830/f12-replay/report.json` and
`f12-independent-pixels.json`; the six approved current RGB hashes are those
in `.analysis/intro-exact-20260830/f12-directory-audit/report.json`.

The formation asset fixture update also passes independent review and replay.
Both valid and deliberately corrupt graph-only packs now include the actual
mandatory boot resources75/76. The original corrupt graph bytes and specific
formation/play-control error assertions are unchanged. A fresh run of
`test_formation_assets.py` against the candidate executable and full pipeline
pack passed; the test now reaches the graph parser instead of failing early
for missing intro prerequisites. This adds no gameplay parity claim.

## Decompressor length qualification

Fresh original-ROM bytes at `$9F:F121` begin `46 FB 00 2E 80`. I independently
read `$80:C644` (`LDA $0003,Y`) and `$C647` (`XBA`) in both the ROM and Ghidra:
this declares0x2E80=11,904 output bytes. The existing host helper incorrectly
read the word at+2 as little endian, reporting0x2E00=11,776. Therefore the
earlier11,776-byte match proves only that prefix, not the full Mode7 character
plane or decompressor completion. Full output length/termination remains
open pending the newly traced decoder repair. Asset75's production resource
comes from the complete attested native VRAM, so its bounded303-frame RGB
comparison is unaffected; no full decompressor claim follows from it.
