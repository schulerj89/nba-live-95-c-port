# Tip-off C image attribution

Full-suite v15 passes the repaired startup matrix and closure, then fails the
old frame90 C image hash. Three privately retained90/170/220 screenshots show
the same expected FORMATION/POSSESSION/LIVE debug phases, with changed player
team presentation and later movement. Only these three C RGB expectations and
their comment change. All original asset hashes, schema checks, court catalog
checks, required debug fields, full handoff and selected-home journeys remain.
Independent review of this new image checkpoint is pending.

The original primary CLI remains unchanged, SHA256
`18fea1fa239680de2337bf0c4bdfd97085fd40f6e315b1cf2a681e0c8e138694`.
Its actual hash is recorded mechanically in `captures.json`; the retained
primary run reproduces each old RGB value below with the same candidate pack
as current. Matched52c2899 private CLI builds differ only in the tipoff source:
one restores exact historical caa9134 tipoff, the other retains canonical
team/rank source. Every other source, header, object and pack is held constant.

| Frame | Historical tipoff RGB SHA256 | Canonical tipoff RGB SHA256 |
| --- | --- | --- |
| 90 | `39d3e6bf4d77fc58280d3e604e03063e82a621124a5d42d3c7f2a1e818b05ae8` | `176c32401147f2222163c745e7e5555fd1604722230d6ad533ebf0bfb89f7b97` |
| 170 | `da83d08b002c789af5b214b02e38d8ee6d1673d87e2d3753c3bb26a0634ff43d` | `95a0a41026b80983219d57bd2c4b310dfa9f72d32299ff9580fcc4bc7aea2764` |
| 220 | `85e8824265afa48bd41005c4d397b0db3e74482a21e11ef96ecfca82453e4e6d` | `97f3f4ff835de26b8ba76e3c3f44c6e224999690301c91a30d77efd05f4eb326` |

The old-source52 build reproduces every primary pixel. The canonical52 build
reproduces every current pixel. Thus later input/controller, inbound or
startup-fixture repairs did not cause these three changes. The source
team-context/rank correction alone explains them. Its original-ROM store
evidence and independent identity checks are documented in
`completion-mode1-attribution-audit.md` and `controller-ownership-model.md`.
The court/ball/player asset bytes are not rebaselined.

Retained artifacts in `build/tipoff-image-attribution-v1` include twelve BMPs,
their PNG previews, gameplay traces/debug logs, exact commands, private CLI
builds and complete pixel comparisons. Root inspected old/current90 and
current220 images: team jerseys/player identities change, and the later
camera/player composition follows the corrected ratings/ranks. These are
existing C captures, not new original-ROM screenshots.

The static score panel still says WEST/ORLANDO with captured score/clock values
that are not the live matchup state. That is an explicitly unresolved port HUD
consumer, not an approved original-game bug or evidence of live score parity.
This image refresh does not approve the panel, human play, whole-frame native
parity or a completed game. No raw native witness or historical failed image
is overwritten.
