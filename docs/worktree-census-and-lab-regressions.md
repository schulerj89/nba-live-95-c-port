# Worktree census and remaining debug input regressions

The reporting tools now accept an explicit read-only capture root. The build
also accepts a recomp root for its census check, so an isolated worktree can
use retained evidence without copying it or silently losing its denominator.
Without any executed ranges, `progress.py` reports N/A percentages instead of
dividing by zero. It does not report absent evidence as100% coverage.

The shared-capture report retains29438 captured address positions and11529
verified positions. Its documented count changes from29091 to29101 because
the current source comments cover ten additional positions in bank82. This is
documentation coverage, not new verified credit or a whole-game completion
percentage. The previous report is retained in
`build/progress-before-capture-root-v1.md`.

The full-ROM census was initially passed the external instruction listings
but still read execution ranges from the empty worktree. Passing the same
capture root through that report restores the exact existing numbers:60346
decoded starts,27478 observed starts and11526 observed-and-verified starts.
All48 bank rows and the complete Markdown report remain unchanged. The JSON
changes only its census-tool source identity, because that tool gained the
path argument. No ledger entry, evidence policy, feature estimate or native
fixture changed. `build/project-census-shared-v3.log` records the full census
regression pass, including regenerated reports checked against stored ones.

Full suitev9 passes core safety, Team Select and Player Setup, then finds two
Player Lab tests expecting two automatic presses in two frames. The real
input contract places the mandatory release between those presses, so these
two state-only checks now run three frames. The retained trace shows Right,
release, Down; team28/roster11 wraps to0/0. The complete Player Lab test passes
with all existing image hashes unchanged. Evidence is in
`build/player-lab-wrap-input-v1.csv` and
`build/player-lab-driver-migration-v1.log`.

These corrections affect testing/reporting only. The full suite has not yet
passed end to end, and the known Rules reentry mismatch remains unresolved.
