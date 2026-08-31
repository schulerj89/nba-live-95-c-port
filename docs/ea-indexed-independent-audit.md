# Independent audit: indexed EA motion/resource checkpoint

Verdict: **PASS for the current candidate's302 phase-aligned rendered frames
and documented static-resource construction. FAIL for release-gate integrity
and whole EA-intro completion until the exclusions below are resolved.**

This review built the implementation independently, read the actual ROM,
Ghidra/recomp output and captured native files, and compared every pixel. It
did not accept the implementer's PASS report as the oracle.

## Scope and reproduction

Implementation: `.analysis/worktrees/intro-transitions/src/nba_ea_intro.c`,
SHA-256 `c1dac7a034055dbd37b00ba254627d7c696c598d10aab491d3fd0590d96ba770`.
Independent executable:
`.analysis/ea-independent-audit-20260830/build/nba95_port.exe`, SHA-256
`57876c8f8f26e17dd8aa64649d4894bdd8bbdd8b8e2285ecbeeb7162230dc704`.
Candidate pack:
`.analysis/intro-exact-20260830/nba95_assets_ea_indexed_candidate.pak`, SHA-256
`54d9955d34229123dc8393a414f3ec34461ab6b874440097be4d21b7530c567e`.

Native source is untouched cold boot with no controller input or machine
writes, captured by Mesen from the original ROM. All artifacts in
`capture-v4/manifest.json` were independently checked for exact size and hash;
manifest SHA-256 is
`8121eebf0e84bec702ba351121f3df7460471a0e513aecda5ba67a7d394071eb`.
The capture script uses synchronous `getScreenBuffer`, not asynchronous
screenshot files. Private portable executable/settings, empty initial saves,
and observed Lua home are recorded. Original ROM and Mesen identities are
the same pinned versions documented in the ownership audit.

Native `$82:F2EA` establishes motion0. The tested mapping is native
motion1..302, native license-relative frames541..842, to C frames346..647.
This is an explicit phase alignment, **not equivalent cold-boot timing**.
The independent run captured a fresh C sequence and compared all57344 RGB
pixels of each of302 consecutive frames to native buffer rows7..230.
Result: **302/302 exact**, no tolerance and no C-derived expected hashes.
The final part of this range is a static hold; the denominator does not mean
302 different actions or complete intro coverage.

Evidence and per-frame hashes:
`.analysis/ea-independent-audit-20260830/independent-report.json`.
Motion7,56,90 image pairs were also inspected visually, including zoomed E,
fixed EA objects and incoming Sports text. The original bank80 and bank82
reference dumps were checked byte-for-byte against the pinned ROM.

## Source and resource review

Read native `$82:F15C-$F4C3`, `$F4C4-$F67D`, tilegroup helper `$80:8FA3`,
and corresponding generated recomp callers. The implementation preserves
observed resource publication at motion23/33/56/66/67, palette publication
69/130, matrix increments12 and clamp256, and the eight-wait flash loops.
`$F64A` stops remaining inner steps when the palette reloads; the C helper
does likewise. Mode7 vertical products truncate separately, matching the
observed hardware output. Original OAM is interpreted as indexed objects;
Mode7 BG1 is composited over their priority0 pixels.

The new71,674-byte asset75 was rebuilt independently while instrumenting
binary file reads. It read only the original ROM and five attested raw
memory resources: `ea_000.vram`, `ea_056.vram`, `ea_000.cgram`,
`ea_023.oam`, `ea_056.oam`. No RGB, PNG, BMP or WAV input was read.
Rebuilt asset SHA-256:
`90800623cd1734cfb41523cb3c89a428d542cb44dfefb12c9c4a35fdf3a3a514`.

A host-decoded11,776-byte PREFIX from `$9F:F121` matches native odd-plane
VRAM. This is not the full character-plane proof: original header46FB002E80
and `$80:C644`/+3 then XBA declare11,904 bytes; the old host length reader
used+2 incorrectly. Full decoder completion remains unverified. Tilegroups `$AD:FF46` and
`$82:F6D8`, plus four palettes `$AF:F05C/$F0BC/$F0DC/$F0FC`, are direct ROM
data. Static base map/OBJ data are ROM-produced native resources, not rendered
images; their format-$30 decoding remains independently unverified.

Compared with accepted pack `5d364ce9...`, the experimental pack removes
IDs3,4,5,6,70,71,72,73,74 and adds75. Every other asset's metadata and bytes
are unchanged. This is a portable indexed-resource renderer, not an emulator
wrapper, and it does not store an animated sequence of RGB frames.

## Exact remaining failures and caveats

1. `build_intro_indexed.py` initially used ordinary `json.loads`, which
   accepts duplicate keys. It trusted `accepted_capture` without independently
   requiring the natural-capture scope, successful exit, executed-script
   identity, launch settings and isolation contract. The current files passed
   this independent audit, but the reusable builder must fail malformed or
   incompatible provenance. Corruption tests are required before release.
2. The initial C resource reader checked magic, low16 version, total size and
   tilegroup shapes but did not validate the complete six-word header. The
   expected schema should be checked exactly. This was not a pixel mismatch
   in the reviewed candidate.
3. The production extractor had not yet been switched to the new asset;
   the audited pack was assembled by an explicit experimental repack. The
   normal extraction/build path must reproduce it and remove old image reads.
4. The decompressor still had a general silent-success path for unknown
   opcodes/instruction-limit exhaustion at review time. The exact Mode7 byte
   comparison protects the checked prefix only, not full output completion
   for this source or any other compressed resource.
5. Native motion0 and303..554, the full native hold, skip/restart behavior,
   caller/return timing, legal/license screens, title handoff and audio are
   excluded. The existing C scene timer and assembled intro WAV were not
   accepted by this rendering audit.
6. Resource-state evidence covers135 native CGRAM/matrix observations and11
   full VRAM snapshots in the implementer's separately reviewed prototype.
   These are not135 direct C routine-entry state replays. The independent C
   evidence here is302 complete final RGB frames plus source inspection.

No whole-intro, all-assets, audio or game-completion claim follows from this
bounded PASS. Renderer changes after the recorded source hash require a new
independent replay; builder-only hardening can be checked separately against
the unchanged resource payload and preserved native evidence.
