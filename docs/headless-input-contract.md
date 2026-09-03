# Headless controller and configured transition contracts

The CLI now calls `nba_game_input_update` with a held button word every frame,
as the GUI does. Automatic commands are one-frame taps separated by a release.
The native menu producer still owns repetition and simultaneous-word behavior;
it was not weakened to accept pressed-only synthetic events. Team Select's
`--team-action-gap N` counts idle frames, including the mandatory release, so
successive pulses remain N+1 frames apart. Fixed `--title-press N` uses zero-based
frame N and cannot be mixed with automatic navigation. Contradictory automatic
scripts are rejected before frames or trace files are produced.

For exact held/combined schedules use `--input-script FILE`. Each non-comment
line is `duration hexadecimal_native_word`, e.g.:

```text
# Complete native SNES words, not host button enums.
400 0000
1 0400
1 0000
40 0100
1 0300
1 0000
```

This waits400 frames, taps Down, releases, holds Right40 frames, changes to
Left+Right without release, then releases. Adjacent equal words remain held;
the driver never inserts releases into explicit schedules. Exhaustion releases
the last word. All12 SNES bits are accepted; low four bits, malformed tokens,
zero durations, more than8192 records or2000000 total frames are rejected.
An explicit script cannot be combined with automatic button scripts. Scene-only
entry flags remain explicitly controlled C entry, not cold-boot parity.

`--input-trace FILE` records every stepped input, all committed/working/Custom
configuration words and controller0's diagnostic repeat state. The native-word
column uses SNES encoding; held/pressed/released use host masks. Post-tick
configuration reflects that frame's actual scene dispatch. Non-Setup working
columns are -1. The strict verifier checks every frame's input latches against
the supplied native schedule, and every stable menu checkpoint against native
data kept outside the CLI process. It does not claim all repeat diagnostics
are native frame-aligned; the separate1770-observation gate owns that boundary.

## Native evidence and independent acceptance

`tools/test_headless_input.py` passes730 native stable configuration checkpoints
across seven journeys and57391 exact held/pressed/released input frames. The
native fixtures are unchanged. It separately checks five automatic C menu
journeys, including exactly two actual visits/returns, ten malformed scripts,
and six preflight rejection cases. `test_headless_input_integrity.py` supplies
seven independent synthetic protocol mutation tests. These are not native data.

The independent audit initially rejected a scene-union crash, ignored missing
arguments, misreported committed-versus-working Main state, silent conflicting
button scripts and permissive trace columns. These are corrected host defects,
not original-game quirks. Original native input release/repeat quirks remain
preserved and commented in `nba_menu_input.c`. The final independent audit remains in Git history; owner logs/reports are
under `build/headless-input-v5/` and `build/config-resume-v1/` in the integration
worktree. Whole-game completion, human play and configured runtime effects are
not accepted by this checkpoint.

## Historical Rules comparisons with explicit configuration

Fresh defaults remain Exhibition/Arcade/Rookie/12min. The optional
`--setup-simulation-three-minute` performs actual Down,Right,Down,Down,Right,
Up,Up,Up taps from settled fresh Main. Working state becomes[0,1,0,0] while
committed state remains[0,0,0,3] until actual submenu/match confirmation. The
flag never assigns configuration, cursor, VRAM or scroll directly.

The original native fixtures retain their historical `port_step` labels.
Only the verifier's explicit C input/mapping changed:

| Contract | Actual C mapping | Independent native comparisons |
|---|---|---|
| Reveal/hold | A187 instead of167; add20 to historical C steps |71 reveal +137 hold frames; same208 consecutive native546..753 |
| Whole first opening |713 idle frames instead of717; A remains884, pre-entry BG2v258 |147 native470..616 frames,22 mapped PPU fields |
| First return | Start547 instead of527; add20 to historical C steps |171 RGB/133 PPU rows each, unchanged and row2/Custom |

Eight configuration taps add16 frames and the four Main cursor releases add4.
For the long whole-open idle, configuration finishes before the idle deadline;
the four cursor release frames replace four idle frames. This preserves the
independently established absolute A884 boundary and BG2 phase, not a fitted
production delay. For Custom return, three row/value release frames replace
three idle frames (209→206); unchanged return retains212 idle frames. Native
values/hashes and production transition timing are not modified. Retained
phase diagnostics: `build/transition-driver-phase-v1/`; exact replay logs:
`build/rules-{open,reveal,settled,return,custom-return}-driver-v1.log`.

Repeated Rules entry remains failing. A precise scheduler repair is separate
from these input-only mappings; no extra production frame skip is authorized.

## Fresh-worktree regression preflight

The first full suite attempt failed before ball-prefix comparison because Git
converted `ball_init_fields.def` to CRLF in the new worktree. Its601-byte Git
blob and original primary file both hash742db6...6706, matching the unchanged
native fixture; the converted622-byte file hash was ae69fb...b1ad. An explicit
LF `.gitattributes` rule now preserves the evidence-bound schema bytes. Neither
fixture hash nor verifier acceptance was relaxed. The subsequent run passed
the complete ball prefix projection/full128KiB unexpected-write guard and
continued until the historical Mode1 C-frame1000 winner-count assertion.
That trajectory attribution is still under independent review; the full
combined suite has not passed and main/desktop is unchanged.
