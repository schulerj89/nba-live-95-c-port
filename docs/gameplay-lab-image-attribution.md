# Gameplay Lab C image attribution

The unchanged debugger gate first failed its frame170 RGB anchor after every
preceding row-count, actor, visibility, CPU-only control, foul and telemetry
schema assertion passed. The only test change is the expected RGB string and
its explanatory comment. Independent audit of this checkpoint is pending.

Old RGB SHA256 is
`d6e5e07757e57bcaa961ac650283d829f4d157a8fae75e26a73380ac71072015`;
new RGB SHA256 is
`d52fd6308d82e0a60ecfcdf86648dd331c40da5c1c2e20b37c67225c0cb1dc6f`.
Four retained CLI runs use the identical candidate pack, frame170, actor7 and
page2. The unchanged primary CLI and a matched52c2899 build restoring historical
tipoff source reproduce every old pixel. The matched52 canonical-tipoff CLI and
current v15 CLI reproduce every new pixel. The two matched builds differ only
in the already accepted canonical home/visitor and sorted lineup-rank source.
Their full build provenance is bound by `build/tipoff-image-freeze-v1.json`.

The corrected team identities/ranks change jerseys, player/camera positions and
rank text. The selected actor, marker and debug panel retain their layout. Old
and new images were inspected. This is a C visual regression anchor, not a
fresh original-ROM screen or proof of complete native frame timing. The static
WEST/ORLANDO scoreboard behind the debug panel remains an unresolved port HUD
gap; it is not accepted as an original-game bug.

`build/gameplay-lab-attribution-v1` preserves all four images, traces, logs,
commands and executable identities. The complete debugger test passes with the
new anchor, including pause/three-step behavior, comparator positive/negative
cases, split actor-pass coalescing and source/input marker assertions. An AST
comparison proves all other test nodes are unchanged. No production file,
asset, raw native witness or earlier failed result is modified by this update.
