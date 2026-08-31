# Independent transition audit, 2026-08-30

The gameplay auditor independently inspected the original ROM, the Ghidra
listings, fresh recomp output, the changed C compositor and setup renderer,
and native/C frame data. This is not acceptance based on the transition
implementer's report. Exact source and artifact hashes accompany the local
comparison in `.analysis/transition-auditor-20260830/`.

## Decisions and scope

| Bounded claim | Decision | Evidence / limitation |
| --- | --- | --- |
| RGB555 brightness must precede RGB888 expansion | PASS | Exact installed Mesen source and retail-ROM mid-fade pixel witnesses agree; old C/Python implementations shared the same error. |
| Incoming Rules reveal, matching Simulation/3-minute configuration, native frames 546–616 | PASS | A fresh independent C run matched all 71 consecutive full 256x224 active frames exactly, with no masks, tolerance or per-frame alignment; 22 exposed PPU fields per frame also match exactly. |
| Construction-to-reveal boundary at native frame 546 | PASS after correction | The first candidate differed at 39,535 pixels. Recording the actual forced-blank flag, including an explicit validity bit, corrected this frame without adding a fade/delay or fitting a route-wide cutoff. |
| Rules reveal through unchanged settled menu, native frames 546–753 | PASS after correction | Independent fresh output from the production-only pack matches all 208 full frames: 11,927,552 pixels. The first settled candidate failed; preserving the native viewport callback and idle arrow palette resolved the actual causes. |
| Four additional Rules UI states at native753/C450 | PASS, snapshots only | Natural captures for row2/right1 and rows7,9,12/right0 match all229,376 pixels across four independently rerun C snapshots. Different button schedules mean these are equivalent-state witnesses, not whole navigation replays. |
| Complete Game Setup→Rules opening from equivalent pre-confirm state, native471–616 | PASS after correction | Fresh independent C run matches all146 frames, 8,372,224 pixels and3,212 PPU fields. Fixed717-frame test-input idle wait matches the native pre-open background phase; production transition timing is unchanged. |
| Rules return, repeated entry/exit and intervening navigation with altered values | NOT ACCEPTED | The full opening and four state snapshots do not prove these separate paths. |
| Whole frontend or whole game complete | FAIL | See the ownership, gameplay and options audit inventories. |

The auditor independently ran
`.analysis/transition-ownership-20260830/private-build/nba95_port.exe` with
that directory's `candidate.pak`. Its 71 actual output frames and telemetry
are retained in `.analysis/transition-auditor-20260830/final-candidate`.
The exact compared binary/pack/source hashes are frozen in
`final-candidate-comparison.json`; filenames alone are not version identity.
Old `c-rules-raster` is a different configuration and is not this result.
The experimental candidate pack also changes a return trace outside this
audit and was not promoted wholesale. The independent auditor parsed both
production pack tables and confirmed that the actual checkpoint pack changes
only asset IDs 144 (Rules-open CGRAM) and 145 (Rules-open trace). Its SHA-256 is
`126b7c8178451dfc76bb0200e3df3f41e7a26e647ee595517893e60b42c8b0c9`.
A separate run against this production-only pack again passed all 71 native
frames and exposed PPU fields.

## Independent reference and capture inspection

The retail ROM SHA-256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Installed Mesen executable SHA-256 is
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`,
version `2.1.1+137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1`.

The old capture launcher's purported save isolation was ineffective.
Mesen's exact-version `ConfigManager.ProcessSwitch` accepts no path separators
in option values, and `ApplySetting` has no string-setting implementation.
`--preferences.saveDataFolder="C:\..."` therefore did not install the requested
path. The default Windows home is the Windows Documents folder's `Mesen2`,
which on this machine resolves through OneDrive. A personal NBA Live SRAM
file existed there. The earlier Custom/12-minute baseline cannot be described
as fresh SRAM simply because its command included that ignored argument.

`tools/capture_setup_transition_exact.ps1` now copies the installed executable
into a new private output directory and writes a portable `settings.json`
beside it. Mesen's home-folder selector explicitly prefers that file. Save
paths are real JSON strings; the save directory begins empty. Controller 1,
zero RAM initialization, fixed video filter/color controls and 224-line SNES
viewport settings are explicit. The immutable script, settings, executable,
ROM, arguments and resulting SRAM hash are recorded in each manifest. The
capture changes no ROM, CPU, or WRAM state. User saves/settings are untouched.

The first minimal private configuration accidentally omitted Controller 1's
type. This caused all scripted input to be ignored while the shared `$80:A2BF`
builder still ran in the title sequence. Its completion sentinel was not
valid frontend evidence. `setup_rules_canonical/INVALID_CAPTURE.txt` preserves
that failure. The runner now explicitly supplies the controller and requires
the actual requested menu handler (`$81:D318` for Rules or `$82:8CD1` for
Options) in the native execution trace.

`setup_rules_canonical_v2` is the successful fresh-SRAM natural capture. Its
native default main values are `[0,0,0,3]`: Exhibition, Arcade, Rookie,
12-minute quarters. Rules are `[0,0,1,0,0,0,0,0,1,1,1,0,0]`; options are
`[30,30,2,1,0,0,0]`. These differ from the C port's Simulation/3-minute
defaults. That is an explicit separate compatibility gap, not a reason to
silently alter a capture or claim matching initialization.

For the bounded reveal comparison, `setup_rules_simulation_v2` uses natural
controller input to choose Simulation/3-minute values. It waits 400 native
frames, issues actions at 60-frame intervals, then rebases only the harness's
evidence labels after 920 frames. Emulator execution continues uninterrupted.
The manifest and capture log disclose the schedule and rebase; no savestate or
state injection is used. The earlier `setup_rules_simulation_v1` attempted
normalization too early, missed inputs, never entered Rules and was correctly
rejected by the handler guard.

The new capture also disproved an overly strong validation assumption:
before entering Rules, edited main values are in working words `$16FB..$1701`,
while committed `$17AB..$17B1` still hold the old defaults. The handoff commits
the edited main values, then reuses the working buffer for Rules. Validation
now compares **before-open working values to after-open committed values**.
Both are `[0,1,0,0]`; before-open committed values remain `[0,0,0,3]`.
The preserved v2 manifest discloses this guard correction and the independent
validation of its unchanged snapshots. A menu label alone was not accepted.

## Frame and state comparison

The auditor read Mesen's exact-version `LuaApi::GetRenderedFrame` and
`TakeScreenshot`. `emu.takeScreenshot()` reads asynchronous video-decoder
output and can repeat an old image while state advances. Those PNG files are
illustrations only. `emu.getScreenBuffer()` synchronously renders the current
PPU frame and supplies the compared `.rgb` stream.

Raw output is 256x239. Rows 7 through 230 are the complete normal 256x224
active viewport, as specified in the private emulator configuration. All
57,344 active pixels are compared; excluding the normal output border does
not crop away game artifacts. RGB comparisons use native frame = C step +303,
established by the opening input/trace boundary and fixed for the entire
interval. There is no nearest-frame search, tolerance, hidden region or
updated C golden in this audit.

`final-candidate-comparison.json` contains each native/C frame's SHA-256 and
different-pixel count. The exposed state projection includes forced blank,
brightness, main/sub layer enables, and each background's horizontal/vertical
scroll, tilemap/CHR addresses, and width/height flags. Native map/CHR word
addresses are converted to byte addresses before comparison. All 1,562
compared fields and 4,071,424 pixels in frames 546–616 match. End-frame register equality alone is
not sufficient; full pixels independently test the resulting scanout.

The auditor visually inspected the native mid-fade logo and the moving Rules
reveal, including native frame 601 against C frame 298. Image conversions in
the auditor directory only encode captured RGB for inspection; they are not
production assets. Production changes use ROM-derived tile/palette/transfer
data, not these PNGs or emulator-composited images.

## Full opening and pre-call alignment

The first full-opening attempt correctly failed because the pre-call states
differed: native BG2 vertical scroll was258, C was19. The native capture's
normalization journey waits400 frames and uses60-frame input gaps before the
harness-only label rebase; the C shortcut reaches its inputs earlier. Applying
the original three-frame/pixel cadence gives `(258 - 19) * 3 = 717` frames.
The CLI-only `--setup-menu-delay717` waits before scripted inputs and changes
no production rendering, blanking, fade or transition duration. This value was
derived from state before pixel comparison, not searched for against images.

The auditor independently read native `menu_transition_ppu.txt`: frame469
and470 have BG2v258; frame471 retains258 and472 advances259. The fresh C trace
has258 at steps884/885 and259 at886, establishing the same divider phase as
well as scroll value. The whole-open mapping is fixed `native = C - 414`,
frames471–616 versus C885–1030. The prior no-idle incoming mapping remains a
separate71-frame contract; neither mapping changes within its comparison.
This proves a transition from equivalent pre-confirm rendering state, not
that the earlier title/normalization/menu journeys consumed identical time.

The state-aligned attempt initially still failed14 frames. Independent source
inspection confirms the causes: `$81:C41E–$C448` clears window register`$2125`
before calling `$80:EBF9`, so the outgoing selected row loses its yellow
highlight despite `$212D` remaining4. `$80:EBF9–$EC67` retains the original
`$EF94` moving IRQ window while scroll increases14 per frame, then writes
`$212C=$13` and retires that IRQ; later resource uploads are not visible BG3.
The updated renderer applies those native state changes without hiding them
under extra blanking. The auditor read the Ghidra bank80/81 listings and the
original-ROM bounded recomp, inspected the changed C source and viewed a
native/C outgoing frame pair after checking their exact pixels.

`full-open-final/independent-comparison.json` records all146 frame comparisons
and22 PPU fields per frame. All8,372,224 pixels and3,212 fields match. The
audited executable hash is
`d18997f856eb2ec81de1a830945d97018677600c9839efa88e7db94a8797e590`;
the production-only pack remains`126b7c8178451dfc76bb0200e3df3f41e7a26e647ee595517893e60b42c8b0c9`.
The auditor reran the137-frame unchanged-menu hold and all four UI snapshots
after this outgoing fix; all still pass independently.

## Owning routines and color evidence

The independent Ghidra reads are
`.analysis/gameplay100-closure-ghidra/gameplay100_bank80_listing.txt`,
`$80:EF94–$EFCF`, and its bank81 counterpart, `$81:D6E8–$D755`.
Fresh bounded recomp/decodes from the original ROM are retained as
`reveal_bank80.c/.txt` and `reveal_bank81.c/.txt` in the auditor directory.
These are readable instruction translations, not executed whole-game
recompiler parity. The older Setup/Team Select `$80:A3B8` listings contain
incorrect M/X decoding and do not prove this reveal contract.

`$80:EF94` waits for HBlank, writes `$212C=$17`, installs callback `$EFB2`,
and schedules the moving edge from `$1777`. `$80:EFB2` waits for HBlank,
writes `$212C=$13`, installs `$EF94` and schedules VTIME=1. The Rules viewport
then uses `$81:D6E8`, `$D70E`, `$D738`: header scroll zero, row scroll from
`$1661/$1662`, and disable at VTIME `$CA`. The last callback ends at `$D755`.
Native frame 601's raster trace independently shows `$212C=$17` at scanline2
and `$13` at scanline143. This is why the current render cannot infer BG3's
whole-frame visibility from the final layer-enable byte or the OBJ-enable
bit. The moving edge is native clipping behavior, not a cosmetic crop.

Exact-version Mesen `SnesPpu::ApplyBrightness` scales/quantizes each RGB555
channel **after color math**, then `SnesDefaultVideoFilter` expands to RGB888.
The auditor independently read those files. In retail
`setup_rules_exact/open_step_552.rgb`, brightness7 gives red18→66 and
red16→57, versus old C values69 and61. The corrected C converter preserves
this order; the previous Python oracle shared the same mistake, so its old
agreement with C was not independent proof. No invented fade, added delay,
blanking interval, production screenshot or tolerance was accepted.

## Remaining conditions

The static forced-blank cutoff failure at frame546 is resolved. A further
natural capture, `setup_rules_simulation_hold_v1`, retains the same menu values
through native frame829. The initial independent run passed frames546–616,
then failed every frame617–753. First-handoff differences totalled612 pixels;
frame753/C450 differed at621 pixels in x16..158, y184..223. The Rules renderer
did not continue the original `$81:D738` viewport after transition completion,
and its static captured arrow retained the pressed palette. Those failures
remain recorded in `production-hold-comparison.json`; no projected C golden
was accepted as native settled evidence.

After both causes were corrected, the auditor ran a fresh 450-frame C journey
with the production-only pack. All208 native frames546–753 match C243–450,
including all11,927,552 active pixels. The unchanged offset remains303.
`production-hold-arrow-comparison.json` preserves every frame hash, native
manifest digest, pack digest and executable digest
`2651f81ef8aa1c4a76960549efa6b0e738c998e031fa2bb1c63f4a3f33d88181`.
The native753/C450 RGB SHA-256 is
`5282ebb046c621a3225df8c96b2b40b5908021bba0b2f0650e9be76d59bbbb9b`.

The auditor independently read `$87:8BA6–$8C18` and regenerated its bounded
recomp/decode (`arrows_bank87.c/.txt`). Descriptor `$AF:AA5C` uses idle palette3
and pressed palette2, with `$1759` bits selecting the recently pressed up/down
arrow. `$81:D327–$D337` clears those bits once `$163B` reaches15. In a separate
fresh, uninterrupted natural capture, `setup_rules_simulation_arrows_v1`, a
down pulse at evidence frame700 yields OAM24=`10 B7 0C A4` for exactly
frames701–715, then `10 B7 0C A6` from716 onward. Frames696–700 are also idle.
This independently confirms the original15-frame lifetime; it does not by
itself prove every C input/navigation frame or every scrolled arrow state.

Four additional natural captures, `setup_rules_ui_row{2,7,9,12}_v1`, reach
selected rows using controller input and retain the complete128KiB snapshot
at753. Row2/right1 changes Out Of Bounds from1 to0; all13 before/after Rules
words are verified. Rows7,9,12 use no value edit and retain all13 words. The
auditor regenerated witnesses from these raw captures, independently ran C,
then separately saved/compared the four actual images. All229,376 pixels
match. `ui-variants-final/report.json` retains source/binary/pack/frame hashes.
The auditor also viewed the native/C bottom-row screen pair. Native OAM and
`$87:8BD2` agree on up-arrow Y78; the `$81:D70E` scanline79 content-scroll
boundary must continue while scrolled. The accepted renderer changes preserve
the complete original glyph/shadow extent and native viewport, rather than
adding a crop to hide residual pixels.

Options, repeated navigation and alternate rule/configuration values still
need separately aligned natural captures. The bounded Rules return review is
recorded below. OAM and raster callbacks are not
fully exposed in C telemetry. No claim of whole-machine state parity, audio,
controller ownership, options consumption or gameplay completion is made by
the accepted Rules interval.

The first committed-gate candidate rejected dropped, duplicated and reordered
frame rows, missing PPU fields and an empty RGB hash in independent mutation
checks. It still accepted a changed manifest ROM hash and did not enforce the
runtime ROM identity or subprocess timeout. Those integrity findings were
returned to the implementer and corrected. The auditor read the revised
strict duplicate-key reader, native manifest canonical digest, original-ROM
identity check, immutable source/artifact hash checks, 30-second subprocess
timeout and complete/consecutive C trace validation. The permanent
`tools/test_setup_transition_integrity.py` passes nineteen tests, including all 22
required-state-field omissions, malformed scalar types, altered provenance,
wrong ROM even after a manifest rehash, raw PPU/flag row corruption, and C trace
dropped/duplicate/reordered rows, duplicate/missing headers and malformed row
dimensions. Literal zero-state parser specimens test only parser integrity;
they are not game expectations.
The added settled-witness audit initially found five accepted corruptions:
contradictory hold/targeted-navigation metadata and four self-consistent but
out-of-domain before/after value arrays. The implementer added native range
checks, strict manifest integer types, exclusive capture modes and a strict
single final-state parser. Those same five mutations now fail as required;
the parser also rejects duplicate/conflicting final rows and malformed fields.
The whole-opening contract additionally rejects omitted/downgraded scope,
dropped/duplicated/reordered rows, changed frame alignment and floating-point
frame labels; its717-frame idle value is fixed by the named contract.
The four native UI witnesses and137-frame hold witness pass. The gate compares
all held RGB frames but only the final exposed C page/row/brightness/BG2 state;
it does not claim exact internal state for every settled frame.
Two additional actual C runs with a changed native RGB hash or valid-shaped
wrong native scroll value both failed exactly. Those outcomes are retained in
`production-gate-mutations.json`. These are verifier integrity tests, distinct
from the 71-frame original-ROM comparison.

## Rules return checkpoint: bounded PASS; complete Rules flow FAIL

The independent auditor reran the latest main executable, SHA-256
`35c490b9941d1f6d15bb9dc2d245143a180da8a4cad4e4f47838d2a78bfa69f9`,
against `nba95_assets_rules_return_candidate.pak`, SHA-256
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.
Both native-authored return gates pass. Two earlier independent raw-array
comparisons, retained under
`.analysis/transition-auditor-20260830/return-{hold,custom}-final/`, also pass
every frame, rather than relying only on the implementer's verifier.

The accepted cases are the unchanged Simulation Rules menu and natural
row2/right1 Out Of Bounds OFF (`$17D5`), which changes Style to Custom. Original
captures are `setup_rules_simulation_hold_v1` and `setup_rules_ui_row2_v1`.
Their native Start dispatch at830 maps to C527. All170 consecutive native
frames831–1000 match C528–697: **340 full224-line RGB frames,19,496,960 pixels**.
All22 exposed PPU fields match in the first132 frames of each case:
**264 states,5,808 scalar comparisons**. The remaining38 frames per case are
RGB and final exposed-state checks, not full per-frame C PPU telemetry.
The final C assertion checks parent Mode row0, all four main values and all13
committed Rules values. It preserves the edited OFF value and Custom Style.
No comparison tolerance is used. The auditor viewed the native946/C643 Custom
parent pair and native831 early return frame.

The fixed C waits are212 frames for unchanged values and209 after the three
row/value presses. They align the observed BG2 phase before Start; they are
controller-harness waits, not added production blanking or fades. The earlier
navigation timelines are not asserted equivalent. The accepted return covers
the observed phase-crossing case. The other modulo-three phases still require
their own natural execution evidence.

The reviewer independently checked these native owners against Ghidra and ROM
bytes, not just the implementation comments:

* `$81:D516–D526` commits13 working Rules words to `$17D1`; the common
  adjustment tail `$81:D47A–D494` publishes Style2 at `$17AD`, including
  saturated left/right adjustments. `$81:D54A–D574` clears the highlight,
  waits at `$80:86B0`, retires the menu callback and invokes `$81:C440`.
* `$81:BD0E–BD23` resets the reconstructed main menu's selected row `$1693`
  to Mode and loads its value/maximum. `$81:CF6F` is the Rules-entry reset;
  it is not evidence for the parent return reset.
* `$87:89D5–89E8`, called from the per-frame Setup task at `$81:F9FC`,
  advances `$168F` modulo three and increments `$0613` on rollover.
  Its20 original bytes are
  `ad8f16c90200d0089c8f16ee13068003ee8f166b`.
  `$174B` requests menu/OAM work; it does not own the BG2 counter.
* `$81:BC6A–BCA8` selects and renders each current main-menu value, then
  `$81:BCC5` invokes `$81:A1EE` to publish the generated graphics/map.
  Its upload calls are `$81:A227/$81:A23D → $80:8BA1`. These are the
  appropriate native boundaries for the unresolved Custom resource timing,
  rather than treating a finished image as proof of upload ordering.

The return pack audit found changes only to asset154 CGRAM and155 trace
relative to the already audited Rules-opening pack. Asset153's raw VRAM base
is unchanged. The 19-line main-value copy preserves the real glyph shadow;
it does not import the entire variant screen. In the final Custom screen,
the200 changed VRAM bytes match the original exactly. Copying the full
captured variant delta would additionally import211 unrelated Mode-cell bytes.

**Resource timing remains FAIL.** Reconstructing native VRAM from the initial
raw image and every recorded write gives exact whole-VRAM hashes for all132
unchanged-return frames. Custom still differs at native831–870 and883–926,
despite matching every visible pixel and all exposed PPU fields. It matches
again at927–962. The portable return begins with the default Rules VRAM base,
and its changed main-value publication does not reproduce all intermediate
native writes. This is retained in `return-vram-comparison.json`; the RGB PASS
must not be promoted to whole-resource, upload-timing or routine-equivalence
proof. Correcting this requires carrying the live edited Rules canvas into
the return and publishing the main-value graphics through the native builder
and upload schedule, with pre-call/generated-buffer/DMA observations.

The old captures' `wram_after_back.bin` was observed at native860, while the
transition is still running. It proves committed configuration at that point,
not the final cursor state. Final Mode is supported by later pixels, final C
state and the separate `$81:BD11` publisher. Both raw snapshots were added to
the manifest's artifact hashes during review; unchanged original manifests
are preserved as `manifest.before-return-wram-attestation.json`, with an
explicit post-capture attestation note. No RGB or emulator state was changed.

The return verifier initially accepted four corruptions: an unrelated valid
WRAM digest, floating-point/boolean navigation metadata, an invalid brightness
in the unused opening trace, and a nonboolean forced-blank value. Those same
mutations now fail. `tools/test_setup_transition_integrity.py` now passes25
tests, including complete ordered return-frame shape, all required PPU fields,
native committed-array values/source identity, and all C trace row domains.
The full13-word final C Rules check was added before acceptance. Existing
native fixture replacement requires an explicit replacement flag.

Repeated natural Rules entry is **not accepted**: the next visit currently
has a different build/reveal release, a BG2 cadence difference and stale
Simulation/ON resources after the Custom/OFF return. Its failures are
preserved by the transition owner for the next bounded checkpoint. Therefore
this review accepts the first146-frame opening,137 held frames, four named
UI snapshots and two170-frame first returns only. It does not certify all
Rules navigation, options propagation, gameplay, assets or audio.

## Limited C golden attribution

The main Custom screenshot changed only26 pixels at x147–208,y104, restoring
the glyph shadow omitted by the old16-line value copy. The auditor separately
loaded the old/current C images and raw native Custom frame980, with the same
BG2v30. Replacing only the old image's x138:248,y104 strip with native pixels
predicts the entire current C image exactly. The accepted C SHA-256 changes
from `e6607c9e642194429b9e8de48359e82325ea18b08da7e98619f9d77bc3838a0f`
to `e8f91cea8043243dd2300f9b3e4e4c90a487e3c9c86f0459650eadd3c47c1771`.
`main-custom-shadow-independent.json` records the independent calculation.
The unchanged portion remains the prior C regression baseline; this does not
certify the entire independently navigated screenshot as native parity.

The final-v3 closure digest attribution and bounded Custom caller check were
also rerun independently. All three caller cases (12 snapshots) pass through
real `nba_game` input/tick dispatch after explicitly controlled native
configuration initialization. Native boundary events confirm saturated45
right and0 left still set Custom, a later edit is committed by Start, and
Options adjustments do not mark Custom. Factory defaults, presets,
persistence, timing and runtime gameplay consumption remain excluded.

The closure comparison is explicitly C-versus-C. The auditor independently
compared all912,000 session bytes: exactly6,000 bytes change, one per152-byte
record at the actual Style member offset2, from1 to2; every other session byte
is identical. Raw native WRAM618→620 independently shows Style1→2 as the
working slider changes45→44. Owned gameplay state, semantic telemetry and
legacy state remain byte-identical over6,000 updates. Only the Rules render
sample changes; all57,344 pixels in that sample equal prescribed native620.
The updated input trace explicitly records five Down presses from native
parent Mode row0 instead of the old single Down from incorrectly retained
Rules row4. Five permanent closure integrity tests pass. The narrow Style
exception is therefore supported; it is not permission to waive any other
state or gameplay difference or label the closure digest ROM equivalence.

## Dispatch-frame extension of the accepted checkpoint

The original first-return check began one frame after Start. The missing
dispatch frame is now independently captured and checked; it was not treated
as an automatic implication of the following frames. Fresh natural captures
`.analysis/rules-return-t0-audit-20260830/native-{hold,custom}` attest both
native470 and830 in `dispatch_ppu_states.txt`. The file's SHA-256 is
`9fb1fab80ab24b29e3cf660ca2170be9af0746f34d5136c6af41a5b0a4f00dbc` in both
cases. Its two complete22-column raw rows were inspected independently.
All170 previously accepted raw RGB frames in each fresh return remain
byte-identical to the earlier source captures.

The independently rerun final gates pass **147 opening frames470–616** and
**342 return frames830–1000** across the two cases. Opening compares147
complete PPU states; returns compare266 complete PPU states including Start.
The fixed frame offsets and input waits did not change. The audited private
executable is `bb9c2114ac110ff6f36134a615e7a898b91acdd689f72084a204171aa34b4082`;
the return pack remains `5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.
The teardown now retains the preceding Rules viewport at dispatch until the
native wait resumes; cursor reset begins on the following update. This removes
the previously unchecked early state change without adding a fade or delay.

The permanent transition integrity suite now passes27 tests, adding explicit
dispatch artifact requirements. Existing first-row removal mutations reject
omitted470/830, and omitted or changed dispatch attestations are rejected.
The Custom intermediate VRAM discrepancy and repeated-entry failures above
remain open. This extension changes the bounded accepted frame population;
it does not promote the result to complete Rules-flow or whole-game parity.
