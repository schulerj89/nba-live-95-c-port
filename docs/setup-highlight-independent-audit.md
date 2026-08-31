# Independent Main highlight review

Accepted for the bounded screen-coordinate compositor correction. No defect
found in the changed text-span helper and Main caller. This does not accept
the original game's initial-entry timing, Rules reentry, or the full legacy
Setup regression. No production source, ROM, asset pack, or expected image
was changed during this review.

Reviewed owner snapshot: `completion-owner/build/highlight-freeze-v1.json`,
SHA256 `cdd9c1c9109d169f530773674d1685420c2da5aa2388e711183307ba672633c9`.
The reviewed `src/nba_setup_screen.c` hash is
`9ac8421f923a6aab4a5c5de77a40635f6be667b1b422877f4e0007b945f8b221`.
All evidence below lives in this worktree's `build/setup-highlight-audit-v1`.
At final verification, the owner's attribution document alone had changed
to clarify the native entrance limitation reported below. The exact reviewed
document is preserved as `owner-attribution.md`; the old owner freeze remains
unchanged. The clarified document's hash is
`4e71c650cbc6c514e93ad7dc1c84d92b3b4dffc25196f5be35df5c7b9306d4be`.
All six other owner freeze targets, including source and executables, match.

The reviewer copied the owner's complete `src`, `include`, source manifest,
build script, relevant tools and unchanged Setup fixtures into a private
`snapshot`. `snapshot-sources.json` records every copied file. A clean build
compiled all objects there, followed by fresh attribution and raw Main canvas
probes. No controller-worktree object was linked against owner headers.

The helper now applies color subtraction using destination screen `y` within
the supplied half-open band. Main passes the surrounding renderer's existing
color-math enable and fixed selected-row band. BG3 scroll changes the glyph
destination but not that band. The other menu caller passes `dy..dy+16` when
highlighted: with `y=dy+py`, this retains its previous `py<16` behavior,
including clipping. The unchanged shadow below the band is preserved.

`attribution/report.json` records an independent four-way comparison at the
nine historical frames104/105/118/125/128/130/146/162/166. The old CLI and new
legacy-state probe both equal all nine unchanged full RGB hashes. The fresh
CLI equals the new fresh-state probe at every frame. Only162 and166 differ
between fresh and legacy, each by1116 pixels, confined to the changed Style
and Quarter value cells. The legacy state is explicitly injected into the
C probe for attribution; it is not a native input route or timing oracle.
All36 BMPs and process outputs remain available.

Fresh snapshot regressions also passed:

- 71 Rules reveal RGB/PPU frames and147 whole-opening RGB/PPU frames.
- 137 settled Rules RGB frames and final page/row/BG2 phase.
- First return in hold and custom-row2 modes:171 RGB frames and133 complete
  PPU states each, including final cursor/configuration.
- Main source-tail poisoning leaves complete RGB and65536-byte raw VRAM
  unchanged. Only the private test pack was modified.

These are repeated comparisons against unchanged native fixtures. The71-frame
reveal overlaps the147-frame opening; the counts are not independent gameplay
coverage. Test logs, exact source hashes and fresh binary hashes are frozen.

A separate fresh original-ROM Mesen run (`native-v2`) used a private portable
executable/settings directory and initially empty private save directory.
It pressed Start through Title, then Down to visit all six Main rows. No RAM,
register, ownership or ROM writes were injected. The observed Lua data folder,
settings and final save hashes are retained with the540-frame capture.

The original ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`;
Mesen SHA256 is
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
The capture manifest SHA256 is
`108db4229201ae7cea4cbcbfd93737b388b419f7f2c14a47e9f5df73039c66b3`.

The observed channel prefix `09 26 00 68 7f` selects HDMA mode1 pairs for
`$2126/$2127` from `$7F:6800`. Decoding the first240 scanlines of512 captured
table bytes yields460 populated windows:16 contiguous lines at top70/88/106/
124/156/174, with281/30/30/30/30/59 observations respectively. Every populated
window has fixed color25952, BG3-only subtraction, fixed-color rather than
subscreen math, and color window1 enabled. This independently supports the
screen-coordinate window and color-math contract in the source.

The fresh entrance does **not** directly show a moving glyph crossing an
active highlight. At setup80, BG3 scroll is14 and subscreen BG3 is enabled,
but the HDMA window table is empty. At setup81 the first16-line window appears
with BG3 scroll0. Thus the inference that a fixed window must not follow BG3
is supported; actual original initial-entry crossing/timing remains unproven
by this capture. Earlier assertions expecting ten, then one, populated moving
states failed and were replaced by these explicit observed limits, not by a
tolerance or a native golden change.

The first exploratory capture (`native-v1`) retained only256 table bytes and
was insufficient for all six rows. An initial decoder also wrongly expected
the longer prefilled table to terminate within the capture. Both attempts
remain preserved; `decoded-window-failed-v1.json` records the latter. The final
decoder reads only the first240 visible scanlines, requiring complete bytes
for every consumed pair. These analysis failures do not change native data.

The independent review does not establish real-hardware execution: the
original executable was observed in Mesen. It does not refresh any initial
default-state baseline. The controller19-file and human10-file checkpoints,
their binaries and human link objects were checked unchanged after review.
Root owns any subsequent integration or verifier repairs.
