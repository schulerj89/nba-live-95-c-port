# Native gameplay score panel and clock

Historical WIP investigation follows. The later bounded score/clock/pause
repair is now enabled on the integration branch; see
[current integration scope](gameplay-hud-integration.md). Native publication
timing, unfinished statistics/advertisements and the limitations below are
not promoted to a complete HUD pass.

Status: child resource/formatting translation in progress. The production
caller, publication queue, full lifecycle and frame timing are **not yet
wired or verified**. Do not describe the HUD or gameplay as complete.

The old C gameplay renderer continuously draws a captured BG3 panel containing
WEST/ORLANDO. Native `$87:B99A-$BA53` instead clears its map during court
initialization. `$83:CC10` schedules panel work after a basket; the five
children are `$83:D0AD`, `$D157`, `$D1B1`, `$D1FD`, `$D2E0`. Native context0
is home/right and context1 visitor/left. The new publisher uses these stored
context identities and the original ROM's city strings rather than relabeling
the old captured panel.

## Resources

Dedicated asset286, `NBHUD001`, contains3500 bytes. The initial63 two-bit
tiles are attested native VRAM outputs of `$87:B99A`'s format30 decompressions;
independent format30 decoding is still pending. Palette, city/period strings,
clock font, font row-expansion table and three tilemaps are direct ROM bytes.
The shared `nba_rom_font` module renders the original glyphs and expansion
table. No screenshot, RGB frame, guessed palette or hand-drawn font is a
production input. Builder provenance pins the natural capture Lua/runner,
ROM, private Mesen and original settings, and checks all consumed raw files.

Normal asset extraction and renderer integration remain pending. The private
candidate pack adds only286 to the accepted pre-intro pack; it must not replace
root's newer full pack. Header/domain integrity and general asset-parser
negative tests remain required before promoting this resource.

## Native observations and projections

Two natural neutral-controller matches are captured in private
`build/native-hud-default-v2` and `build/native-hud-alternate-v1`. They preserve
first-court state, publisher entry/exit WRAM/VRAM/CGRAM and at least61 consecutive
native RGB/PPU frames around the first panel. The default first basket occurs
at court970; names publish at974. The distinct team pair reaches its names
publication at1132. These are natural emulator journeys, with private saves
and no state injection; full C scheduling is not aligned to them yet.

The five C child calls replay native pre-call inputs through the actual
publisher. Both cases reproduce all1536 working tilemap bytes,2128 working
character bytes and8 clock-text bytes at every native child exit:10 complete
working-buffer comparisons. `$D1FD` includes its real `$87:BBE9` clock consumer;
there is no fake extra formatter call after the period routine. `$D2E0`
publishes the original `$08E6=$FFFF` completion, and `$87:BACB` publishes
`$08F4=$FFFF` when its original negative-timer branch runs.

This projection is not a final-frame proof. `published_characters` describes
queued upload results; native VRAM observes actual DMA/NMI execution. The
visible map still differs at the sampled D332 return because a queued upload
has not completed. A portable publication queue must determine visibility.

Extending the comparison to canonical state exposes another dependency:
both natural `$D0AD` calls enter with event `$13E7=4` and return with0, whereas
the isolated C child leaves4 unchanged. The routine yields while another
native task services events. The writer is now independently captured: `$82:FDD0` clears the primary
event, `$82:FF4C` clears the secondary `$13E9` event, and their random audio
choices advance shared RNG `$07F6` through `$80:CEF2`. Clearing either bit
inside the HUD would misattribute the behavior. The extended projection also
shows current clock/timer changes during D0AD and name-builder yields. These
shared-task differences remain explicit and block complete parent parity.

## Controlled clock formatter proof

`tools/capture_hud_clock_controlled.py` captures35 controlled native calls
reached through the actual `$87:BBE9` dispatcher after a natural first panel.
Only eight declared WRAM words are injected. They and the clock text are
restored at formatter return before downstream rendering. These cases are
never used as production assets or described as natural match trajectories.

`tools/verify_hud_clock_formatters.py` validates raw provenance and exact case
shape/order, then supplies only native pre-call words/text to the dedicated C
probe. All35 cases match all8 exit words and8 text bytes:11 minute-format
cases at `$87:BAF5-$BB58`, and24 tenth-format cases at `$87:BB59-$BBE8` with
both busy states. This includes final-tenth compatibility and expiration's
`$09B4/$13E7` side effects. It does not cover every possible clock value or
the caller's scheduling. Six synthetic verifier test groups include mutations
of each of560 compared exit words/bytes; those tests establish gate integrity,
not additional ROM parity.

`$87:94A2-$94A5` publishes current clock `$0928` into snapshot `$092A` before
the main gameplay lifecycle consumer. The formatter reads the snapshot,
which must not be replaced by a rendering-time read of current clock.
`$87:BBE9-$BD2E` also consumes `$492B`, shared presentation timer/kind
`$08DE/$08E8`, previous clock `$08F6` and clear flag `$08EE`. Its unsigned
mirror subtraction and comparison are preserved; no host frame delay is
substituted. The canonical `$092A` production publisher and downstream
event/audio consumption still need integration at their actual caller.

## Remaining bounded work

- Capture the D0AD event writer across its yield; keep audio RNG/event
  consumption in the appropriate shared task.
- Wire canonical clock snapshot and complete `$83:CC10/$CE36` parent
  lifecycle, including CPU advertisement/overlay choices and RNG usage.
- Implement observed graphics job/NMI publication boundaries so queued
  resources do not appear too early, then compare61 consecutive native
  frames through a normal basket and subsequent clock updates.
- Integrate resource286 through the normal ROM-derived extraction pipeline,
  validate malformed resource rejection, and replace the stale always-on HUD.
- Obtain independent source/native/caller audit before accepting the scope.

Human control remains disabled pending its separate native ownership and
dispatch work. The HUD translation does not certify player controls, full
match timing, audio, fouls, options effects or end-game behavior.

## Paused WIP handoff

Work stopped at the user's request on2026-08-30; no new gameplay integration
is claimed. `verify_gameplay_hud_children.py` defaults to failing any of the
shared-state differences. Its explicit `--bounded-owned-fields` mode accepts
only the three working buffers and five listed child-owned words; the report
still contains every observed shared-field mismatch and marks parent/NMI
timing FAIL. Both natural team cases pass that limited projection. Full
shared-state comparison fails (five fields across D0AD/name-builder in the
default case), as expected.

The completed read-only `build/native-hud-event-owner-v2` capture also directly
observes `$87:94A5` writing `$092A=$0928` at native lifecycle dispatch, with
the snapshot remaining unchanged during the following suspended D0AD call.
During that call, `$13E7/$13E9` both change4 to0 and RNG changes
0x6874 to0xD0E8 to0xBC57 through the two original audio consumers. No
HUD-local state patch was added. The audio workstream has a separate exact
event translator; coordinated scheduler integration remains future work.

The HUD module, resource builder, capture tools, C probes, exact formatter
gate, verifier tests and this document are saved as WIP. Captures, generated
reports, packs and executables remain local evidence outside git. Root owns
any commit/push; no independent agent push was performed.
