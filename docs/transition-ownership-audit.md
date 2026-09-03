# Frontend transition ownership audit

Established2026-08-30 from the actual checkout, raw captures, native ROM,
source and disassembly. The game and its complete transition system are not
finished. This report distinguishes bounded render parity from full original
routine/caller/resource scheduling equivalence.

## Identity, ownership and artifact map

The initial main/origin revision was2723af610aab0ec63263a6449fa6a161a155f974,
clean after fetch. No AGENTS.md exists in the repository or its ancestors.
The canonical ROM is F:/Games/SNES/NBA Live 95 (USA).sfc,1572864 bytes,
SHA2562115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
Root owns integration, builds, commits/pushes and the broader inventory in
`docs/ownership-plan.md`. This workstream owns frontend Setup transitions and
capture/gate tools; the independent auditor owns its separate audit report.
The intro follow-up runs in root's separate worktree; no agents push alone.

| Artifact | Verified location and role |
|---|---|
| Scene/caller routing | `src/nba_game.c`; screen enums in `include/nba_game.h` |
| Setup UI/render/state | `src/nba_setup_screen.c`, `include/nba_setup_screen.h`; session values in `src/nba_session.c` |
| Other frontend modules | `src/nba_ea_intro.c`, `nba_title_sequence.c`, `nba_team_select.c`, `nba_player_setup.c`, `nba_player_intro.c` |
| Gameplay/lifecycle transitions | `src/nba_tipoff.c`, `nba_tipoff_flow.c`, session and gameplay modules; owned by gameplay/integration workstreams |
| Portable PPU/audio | `src/nba_snes_ppu.c`, `nba_renderer.c`, `nba_audio.c`, `nba_spc.c` |
| Recomp reference | `../NBA-Live-95-Recomp/generated/` currently banks00/80/81/82 only; not a Git repository; bounded generated references under its `.analysis/` and this checkout's `.analysis/` |
| Ghidra | `ghidra-projects/`, `tools/ghidra/`; correct raster dumps `.analysis/gameplay100-closure-ghidra/gameplay100_bank80_listing.txt` and `gameplay100_bank81_listing.txt`; arrow owner in `.analysis/gameplay85-closure-ghidra/gameplay85_bank87_listing.txt` |
| Historical frontend evidence | `.analysis/intro_capture`, `title_capture`, `title_verify`, `setup_capture`, `setup_transition`, `setup_main`, `setup_rules`, `setup_options`, `setup_rules_scroll`, `team_select_ghidra`, `team_select_navigation`, `player_setup`, `player_intro_ghidra` |
| Extraction/production sources | `tools/extract_assets.py`, `tools/snes65816_decompressor.py`, `tools/mesen_*_capture.lua`; production pack is raw ROM-derived tile/palette/OAM/SPC/BRR and decoded resource data; intro violations discovered independently remain tracked in `intro-indexed-resources.md` |
| Fresh exact evidence | `.analysis/transition-ownership-20260830/`; canonical native capture directories and C frame sequences named below; screenshots and RGB remain evidence only |
| Capture runner | `tools/capture_setup_transition_exact.ps1`, `tools/mesen_setup_menus_capture.lua`; strict raw/provenance helper `tools/setup_transition_capture.py` |
| Native gates | `tools/test_setup_rules_reveal.py`, `test_setup_rules_settled.py`, `test_setup_rules_return.py`, their `tests/fixtures/setup-rules-*-native.json` witnesses |
| Deterministic regressions | `tools/test_setup_transition.py`; other `test_*` and C probes are not independent ROM evidence merely because they pass |
| Independent audit | Historical report in Git history; evidence remains under `.analysis/transition-auditor-20260830/` and `tools/test_setup_transition_integrity.py` |

## Screen and return-path inventory

I=implemented, W=production caller wired, R=bounded native verified,
T=C regression tested, V=actual images inspected, A=approximation/legacy trace,
M=missing behavior or materially missing evidence. These are separate labels,
not a progress percentage. No row implies all branches, configurations or frames.

| Directed edge / subsystem | Present evidence and remaining work |
|---|---|
| Legal / EA / title cold boot | I/W/T/A; fresh root evidence finds omitted second legal wait and EA motion divergence; prohibited PNG production inputs must be replaced. See `intro-indexed-resources.md`. |
| Title dismissal -> Game Setup | I/W/T/A; native brightness-only initial-frame corrections accepted, complete normal transition parity still M. Existing delayed screenshot alignment is not proof. |
| Setup -> Rules first configured entry | I/W/R/T/V; exact147-frame configured witness below. Packed trace/caller translation remains A. |
| Rules -> Setup first return | I/W/R/T/V for unchanged Simulation and changed row2/Custom,171 frames each. Other return phases/values and invisible write ordering remain M. |
| Rules repeated entry/exit | I/W/T, native repeat FAIL; wrong live-text source, modulo3 phase and builder completion timing. Second return matches, reopening does not. |
| Setup <-> Options | I/W/T/A; brightness-only changes independently predicted, original scene/resource scheduling and all intermediate frames still M. Existing Options construction guard has no fresh parity proof. |
| Setup -> Team Select | I/W/R/T/V/A for the normal Exhibition handoff: consecutive native frames now anchor the outgoing layer windows and a continuous production capture guards their pixels. Altered settings/team coverage and exact whole-frame parity remain M. |
| Team Select -> Player Setup and return | I/W/T/A; existing logo/navigation fixtures; full transition, cancellation and repeated selections M. |
| Player Setup -> introductions / starting lineup | I/W/T/A; ROM-derived portraits and existing tests; changed assignments/lineups and every normal frame still M. |
| Starting lineup -> tip-off | I/W/T/A; existing presentation and gameplay probes; independently aligned camera/resource/input handoff M. |
| Timeout / substitutions / return | I/W/T/A; gameplay workstream owns exact native scheduling, saved setup propagation and repeated return gaps. |
| Quarter / halftime / overtime / final horn / postgame | I/W/T/A; full natural CPU/user and CPU/CPU journeys and native timing M. A fresh-match session reset is integrated only at new Exhibition start, not period restart. |
| Postgame -> Setup -> second match | I/W/T; bounded new-match reset independently tested; full native visual/end-to-end journey M. |
| Season / playoffs / load-series / disk persistence | M or partial placeholders per gameplay/options inventories; menu labels are not completed game modes. |

## Testing critique and capture corrections

An instruction comment census counts retained executed address positions,
not correct behavior. At the initial commit100%=28643/28643 annotated captured
positions;40.25%=11529/28643 eligible retained positions;19.10%=11526/60346
conservatively decoded starts. All exclude uncaptured control flow, scenarios,
and unknown code. Weighted feature estimates lack an independent completeness
denominator. C-versus-C hashes, copied implementation fixtures and branch-only
probes cannot establish native caller/runtime completion.

Mesen test-runner max speed skips PPU rendering unless DisableFrameSkipping is
true. `emu.takeScreenshot()` reads an asynchronously presented decoder image;
old adjacent PNGs were identical while registers advanced. Fitting a one-frame
trace delay to these PNGs was invalid. Exact captures use synchronous
`emu.getScreenBuffer()` with rendering skips disabled. Native256x239 RGB always
maps to224 active rows starting at fixed row7; no fitted per-frame crop is used.

The original save-folder CLI isolation did not work: the installed parser
cannot apply string paths. The new runner copies Mesen/settings into an isolated
portable directory, explicitly configures Port1=SnesController, and never alters
user settings or kills unrelated emulator processes. Canonical screenshots are
reached with real UI input, not CPU/WRAM state injection. Fresh factory Arcade/
12-minute defaults differ from the C historical Simulation/3-minute defaults;
matching the latter requires real menu normalization before interpretation.

End-of-frame PPU layer bits omit internal IRQ windows. Native raster callbacks
show $80:EF94-$EFCF and $81:D6E8-$D755 enabling/disabling BG3 and OBJ within a
frame. The old renderer treated subscreen and OBJ enable as unrestricted BG3
visibility. Correct native designation/IRQ timing replaces that invented
permission. The early team_select_ghidra A3B8 fragment is incompletely decoded;
its presence is not sound source evidence. The auditor independently checked
canonical ROM bytes and correct listing/recomp slices for accepted changes.

Native witness authors run without the C executable and write oracles before
verification. Existing fixture replacement requires an explicit flag. Strict
JSON, source identities, raw digest attestation, field counts, order, numeric
domains, full row sets and mutation tests prevent missing/duplicate/relaxed
records from yielding PASS. Exact configured gates remain separate from old
C-only regressions, natural versus injected evidence, and incomplete resource
translation claims. See the options workstream for same-dispatch inbound state
capture replacing the compensation envelope.


## Expanded first-visit checkpoint: independent PASS; reentry FAIL

The configured first opening now matches all147 native frames470..616 against
C884..1030 (including the A-confirm dispatch frame), including every57,344 active RGB pixel and22 mapped PPU fields. The
headless input script waits717 extra frames before opening: this was derived
before comparison from native incoming BG2v258 minus C19, multiplied by the
native3-frame/pixel cadence. No production fade, delay, or offset search was
introduced. The first selected-row divergence was native white text versus C
yellow; $81:C41E-$C448 clears the window, while $80:EBF9-$EC67 retires its IRQ at
BG3 scroll182. Subscreen enable alone was incorrectly authorizing visible BG3.

The independent held-menu continuation compares137 RGB frames617..753/C314..450
and final page/row/BG2 state. Four natural settled UI cases (row2/right1 and
rows7,9,12) each compare the final753/C450 image and working rules. These have
equivalent UI states but different earlier button timings; they are not proofs
of every navigation frame. The native arrow pulse is palette2 for exactly15
frames701..715, idle palette3 before/after ($87:8BA6-$8C18 and $81:D327-$D337).
The native viewport ends at scanline203; the old settled path leaked an eighth
row after releasing the packed transition. Up-arrow Y78 and its scanline79 OBJ
boundary are preserved. Rules OFF glyphs retain the native19-line shadow span.

First Rules returns now have two fresh independently captured natural witnesses:
unchanged Simulation and row2/right1 Custom, under
`.analysis/rules-return-t0-audit-20260830/native-hold` and `native-custom`.
Earlier evidence remains preserved in `setup_rules_simulation_hold_v1` and
`setup_rules_ui_row2_v1`. Each compares171 native frames830..1000/C527..697,
including133 complete mapped PPU states,38 post-trace RGB frames, final Mode
cursor, all4 committed main values and all13 committed rules. Thus this return
gate checks342 RGB images and266×22 PPU fields. It excludes intermediate CPU,
DMA-cycle, OAM-generation and whole-machine equivalence.

The return input script waits212 frames after opening (209 plus three row/value
presses in Custom) so both press Start527, matching native Start830 under the
already fixed303-frame relationship. This is test input timing, not game timing.
Fresh unchanged, row2-changed and row1-changed native return PPU traces are
byte-identical; all begin fade-in at888. An earlier concern about different
builder duration came from the stale fallback profile, not those raw traces.
The actual repair removes a one-frame resource/state delay, records true
INIDISP forced blank, retires the old Rules viewport, applies the parent reveal
IRQ, and returns the main cursor to Mode ($81:BD0E-$BD22).

The first audit deliberately inspected the omitted dispatch boundary and found
another visible bug: C527 blanked the Rules canvas one frame before native830.
$81:D54E clears the selected-row window immediately, then $81:D55B waits for
$80:86B0 before teardown. The corrected dispatch render retains the live Rules
canvas and cursor, with the highlight already cleared; the following update
begins the return trace. Fresh `dispatch_ppu_states.txt` captures complete470
and830 register states, separately from the transition stream; the new witness
checks them exactly. `takeScreenshot` PNG830 still shows stale yellow text,
whereas synchronous RGB830 and the port correctly show white text. No PNG is
used as the exact framebuffer oracle. The A-confirm470/C884 frame was already
pixel-identical and needed no production change.


Every native Rules adjustment dispatch marks Style Custom at $81:D47A-$D491,
including a clamped Right at45 or Left at0; Start commits the13 working rules.
The port now preserves that side effect. Its parent Custom glyph cell is copied
from documented ROM-derived2bpp assets before visible reveal. The final Custom
VRAM differs from Simulation in exactly200 independently observed bytes; copying
an entire old capture's delta would wrongly import211 unrelated Mode bytes.
Full Custom VRAM timing is **not** exact: differences remain before native927;
all65536 final bytes agree927..962. These invisible write-order differences
remain an implementation gap even though all corresponding RGB and mapped PPU
states agree. The native cadence owner is $87:89D5-$89E8, called from $81:F9FC:
$168F counts0..2 then resets and increments $0613. The tested return crosses that
phase boundary93→94; other return phases are not yet independently accepted.
$174B controls menu/OAM work and must not be described as the BG2 counter.

The candidate pack is `nba95_assets_rules_return_candidate.pak` under the local
workstream evidence directory, SHA256
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.
Relative to accepted Rules-open pack126b7c…, only assets154/155 change;153 raw
VRAM remains identical. Asset154 is the attested native return CGRAM
`c0f020106386715daa7583eaca351851bf7a4863c2118079ffa7d56bbb2693ec`;
155 is the current-frame PPU/resource trace with observed blank flags,
`5df91ebf3bec4fe9fa346f5d808d73871fe18ac0e07cf19864bc88a0533e75b6`.
There are no PNG, screenshot, or recorded-frame production assets. The extractor
requires `.analysis/setup_rules_return_exact` (or `NBA95_RULES_RETURN_CAPTURE`),
produced using `capture_setup_transition_exact.ps1 -SimulationThreeMinute
-HoldMenu` with a fresh output directory. Original manifests were preserved
before adding the digest of the untouched `wram_after_back.bin`; this disclosed
post-capture attestation binds the committed-state witness to its source.

A fresh natural repeated journey (`setup_rules_repeat_row2_v1`) now explicitly
fails: after returning with Custom/OFF, navigate Mode→Rules again, then toggle
row2 back ON and return. Native second entryA1100/C797 uses132 extra test-input
idle frames between visits so the prior return and subsequent entry retain the
fixed303-frame relationship. Second return1461..1630/C1158..1327 still matches
170/170 RGB frames. Second opening1101..1246/C798..943 matches69/146, with both
live-text and scheduler/phase differences. The native build first fades in at
1176 (A+76), while the first-entry template assumes A+77. Its BG2 modulo3 phase
also differs despite the same entry scroll70. No value-dependent timing patch,
extra blanking, or alternate capture template has been invented to conceal it.
The nonblocking native reentry gate preserves this failure for the next
checkpoint; the whole directed Rules transition is **not complete**.

All other remaining inventory items above remain open unless separately updated
by their owner: Options transitions, all bar settings and return phases, main
presets/Custom storage semantics, Team/Player Setup, intro/gameplay handoffs,
regulation/overtime and other game modes. The independent audit record remains in Git history. Passing instruction or C hash census
percentages do not reduce these scope exclusions.


## Reviewable visual evidence and commands

Under `.analysis/transition-ownership-20260830`, the paired
`rules-return-hold-native-c-30fps.mp4` and
`rules-return-custom-native-c-30fps.mp4` show every original post-dispatch frame
831..1000 with native on the left and C on the right. Playback30fps is for
inspection only, half the native source cadence. Their contact sheets show
consecutive native933..940 at full source resolution. The separate dispatch
captures are under `c-rules-dispatch-hold` and `c-rules-dispatch-current`, with
native830/C527 included; the independent auditor also retains before/after and
rawRGB-derived visualizations in `.analysis/rules-return-t0-audit-20260830`.

The normal extractor was rerun without environment overrides and reproduced
exact SHA5d364ce…; `return-reproduce.log` and `return-pack-diff.json` record it.
All147 opening/342 return RGB frames and corresponding complete PPU states pass
against the fresh executable. The reentry gate remains deliberately separate:

```powershell
python tools/test_setup_rules_reentry.py --exe build/nba95_port.exe --rom "F:/Games/SNES/NBA Live 95 (USA).sfc" --pack build/nba95_assets.pak
```

At this checkpoint it reports204 exact RGB/state mismatches, starting at
native1101/C798. This failure is retained; it is not converted to a passing
smoke test or placed behind a tolerance. The saved native witness has316 rows
and full configuration snapshots bound to attested raw WRAM.
