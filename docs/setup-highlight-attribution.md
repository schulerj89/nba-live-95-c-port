# Early Main highlight attribution

Independently accepted. This is a bounded compositor correction, not full
native initial-entry or reentry timing acceptance.

The original HDMA window at `$7F:6800` gates color math in screen scanlines.
BG3 scroll does not move that window. The Main value overlay instead attached
gold color to the logical selected row, while the existing background/font
renderer correctly used screen coordinates. Preserve the original fixed
window; do not align it to a moving label.
The source evidence for PPU registers and six sixteen-line windows remains
in `nba_setup_screen.h` and its original capture documentation.

The independent fresh native Title-to-Main trace confirms all six fixed
screen bands and the PPU color-math contract. It does not show a moving-label
crossing: at native Setup80 BG3v14/sub4 the window table is still empty; the
first16-line window appears at Setup81 with BG3v0. The crossing seen in the
current C entrance therefore cannot be called an observed original-game bug.
Native initial-entry timing remains unresolved. This correction makes the
value overlay obey the same screen window as the surrounding compositor.

The correction passes screen band bounds to the text-span helper. Main uses
the same color-math enable and bounds as the surrounding renderer. Menu
callers keep their previous local band bounds. No timing, state, asset or
native oracle is changed.

Controlled attribution in `build/setup-default-attribution-v5/report.json`
compares four executions at each of nine historical frames: untouched primary
CLI, current CLI, and a current production-renderer probe with explicitly
declared fresh or legacy configuration. With legacy configuration, the corrected
renderer reproduces all nine historical full-image hashes exactly. Current
CLI equals the fresh-config probe. Only frames162/166 differ from legacy,
each by1116pixels, all within the two actually changed Style/Quarter value
cells after their current scroll. This isolates the remaining baseline change
to fresh Arcade/12-minute defaults. The probe's injected legacy initial state
is C-only attribution, not a natural menu or native timing claim.

Earlier failed attribution attempts remain preserved. An initial hypothesis
that old cursor selection differed was wrong; it was removed. The actual
cause was the overlay/window coordinate mismatch. No historical or native
hash was changed to conceal those failures.

Fresh owner checks preserve the147frame native Rules opening and137frame
settled comparison, plus Main RGB/raw span guards. First-return checks are
recorded separately. See `docs/setup-highlight-independent-audit.md` for the
private fresh build, unchanged native regressions and original-ROM window
capture. Initial-default baseline migration, Rules reentry and the full legacy
Setup regression remain separate work.
