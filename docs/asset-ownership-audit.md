# Independent asset and audio provenance audit, 2026-08-30

**FAIL for the complete production-asset/sequence requirement.** The main
extractor still imports emulator PNGs for legal/EA art and hand-embeds the
Nintendo license bitmap. Most other artwork uses ROM bytes or raw PPU tile
memory, not screenshots. Audio samples are generally ROM-derived, but that
does not establish the original sequencer, timing, mixing or shared RNG.
No source/asset changes were made by this audit; intro replacement is a
separate workstream and must receive its own before/after review.

The auditor read `tools/extract_assets.py`, the actual consumers in
`src/nba_{game,ea_intro,title_sequence,setup_screen,team_select,player_setup,
player_intro,player_lab,tipoff,audio,assets}.c`, the audio tests and their
claimed evidence. The inspected existing pack is272 assets,90,057,659 bytes,
SHA-256 `d6adfe3ab8a49805a2cd10921281c33541135a332b4f2b174dfe25c093c2ebfd`.
The extractor at inspection was
`0794e1b3e9c18636724048db9902e0a781f53a3615076bf0f08f78ff80aa6475`.
The normalized original ROM is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
`.analysis/asset-ownership-20260830/source-and-pack-audit.json` records every
asset's size/digest and these identities. This is a source/provenance review,
not a claim to have reverified every screen or heard every sample.

## Concrete failures and unclosed provenance

1. **Asset1, Nintendo license:** `create_asset_pack` embeds176 literal bitmap
   bytes in `license_rows`, approximately lines987–1004. `nba_game_render`
   displays them through the normal cold-boot path. No ROM address, decoder or
   native source check creates these bytes. Their absence as one contiguous
   ROM substring does not prove they were drawn by hand—the native source
   could be compressed/tiled—but their present provenance is unestablished.
2. **Assets2,3–6,72,73:** extractor lines1010–1123 read `legal.png`, four
   `ea_stage_*.png`, `ea_motion_131.png`, and motion frames56–66. Legal applies
   a grayscale threshold100; EA applies RGB thresholds10, bounding-box
   cropping and transparency conversion. `nba_game` and `nba_ea_intro` use
   these assets in normal startup. These are direct forbidden capture art,
   regardless of matching screenshots or source comments.
3. **Assets70,71,74:** E/A/SPORTS pixels are decoded from raw Mode7 VRAM/CGRAM,
   but their dimensions/origin still depend on the PNG-derived EA bounding
   box. These are not independent of the screenshot pipeline yet. Their
   native tilegroup decode can be retained while removing that dependency.
4. **Production literal palettes:** asset262 packs six literal BGR555 colors;
   introduction fonts use hardcoded black/gray/white and the lineup underline
   is drawn with host yellow. Some are documented observations, not proven
   guesses. The six ball colors independently match raw tipoff CGRAM at byte
   offset458 exactly, but extraction does not check this source and does not
   derive them from the native palette publisher. Establish that path and
   pack its palette instead of silently treating a literal as verified.
5. **Raw-memory source integrity is uneven:** outside the newly hardened Rules
   capture path, most inputs are checked for file existence/length, sometimes
   uniqueness. The extractor does not require original-ROM/emulator/script/
   settings hashes or a completed natural-capture manifest for each input.
   A correctly sized unrelated `menu_vram.bin` or ARAM bank can be packed.
   Setup's `$AE:C446` ROM-decompression cross-check even prints a warning and
   proceeds on mismatch/exception. A versioned pack digest catches changes
   but does not independently establish the source of its existing bytes.

## Complete packed asset-family map

“ROM bytes” below describes the extraction source, not runtime parity.
“PPU dump” means raw indexed memory decoded by portable code, not pixels
returned by Mesen's screen buffer or screenshot API. Such a dump still needs
a verified capture/source contract. Unused/debug assets are kept separate
from assets actually rendered during normal play.

| IDs | Source and actual consumers | Audit result/caveat |
| --- | --- | --- |
| 1 | Literal Nintendo bitmap → `nba_game_render` | FAIL provenance as above. |
| 2;3–6;72;73 | Mesen PNG-derived legal/EA static and fixed-flash frames → normal `nba_game`/`nba_ea_intro` | FAIL production capture art. |
| 70;71;74 | Raw Mode7 VRAM/CGRAM → independent E/A/SPORTS layers → `nba_ea_intro` | Indexed source, but PNG-dependent canvas remains. |
| 7 | Four ROM BRR clips assembled offline into one5.05-second WAV → `nba_game_tick` calls `nba_audio_play_wav` once | No recorded emulator WAV; host-authored sequencing/timing remains. |
| 8–11;18–69 | ROM BRR offsets decoded into standalone WAVs → F11 audio debugger | ROM source, previews rather than production title/gameplay playlist. Decoder/rate fidelity is a separate gate. |
| 16–17 | `.analysis/setup_capture/{vram,cgram}.bin` → Setup base layers | Raw PPU source; ROM cross-check is nonfatal and incomplete. |
| 80–82;87 | Title initial VRAM/CGRAM, per-frame memory/register deltas and cue trace → `nba_title_sequence` | No screenshot pixels. Finite captured behavior replay; old one-frame delta delay justified using asynchronous screenshots needs fresh timing audit. |
| 83–86 | Title ARAM/DSP/register snapshot and APU command trace → `nba_audio_play_title_spc` | ROM-produced program/sample data; native intra-frame input timing lost. |
| 88–93 | Setup first-KON ARAM bank, initial DSP/CPU state, APU trace, entrance PPU trace, DSP trace → `nba_audio_play_setup_dsp`/Setup renderer | No recorded song PCM in pack. Playback uses93, not the captured APU/SPC sequence91. Source/sequence caveats below. |
| 94–123 | Thirty Setup BRR directory entries from asset88 decoded into WAV previews → F11 | Raw BRR previews; normal menu SFX instead initializes the DSP from88–90. |
| 124–129 | Rules/Options settled VRAM/CGRAM/OAM → `nba_setup_screen` | Raw PPU source; only selected states have new independent evidence. |
| 130–142;152;156–157 | Raw VRAM canvases for main-menu and Options values → Setup glyph/value composition | No PNG pixels. Stored canvas selection is not proof that the values reach gameplay. |
| 143–155, excluding152 | Rules/Options opening and parent return VRAM/CGRAM/delta/state traces → Setup transitions | Rules open143–145 now has attested raw inputs and exact146-frame proof. Other routes retain their own pending scope; never inherit that PASS. |
| 160–188;189;192–249 |29 team VRAM/CGRAM/OAM dumps, OBJ logo decoder, captured plate OAM → Team Select, Player Setup, introductions | Portable tile decoding, not Mesen rendered pixels. Captured home/team identities and source integrity still need uniform verification. |
| 250 |26 bytes at ROM file10968, selected-plate palette cycle → team/player menus | Direct ROM bytes. |
| 251 |29×12 native player records via ROM roster pointers → players/introduction/rating/physics readers | Direct ROM source. Wrong team orientation in the C caller remains a separate proven failure. |
| 252;253;254;255 | Raw ROM body/head tiles; ROM-decompressed team/skin palettes; observed OAM layout; fixed default pose |253/254 are actual sprite sources.255 layout is used by F9 default pose.252 is a packed regression image with no normal renderer consumer. Literal255 palette is not the live player's palette—the renderer uses254. |
| 256 | Complete selected animation descriptors/resource graph and attachment/number tables from ROM → `nba_player_lab` sprite/animation functions, live tipoff | Direct ROM assets. Real caller, animation timing, attachment and identity correctness require separate tests. |
| 257–259 | Player Setup raw VRAM/CGRAM/OAM → Player Setup and outgoing introduction base | No captured RGB; snapshot source and transition timing need independent gates. |
| 260;261;264;271 | Court BG layer,290 keyed lineup portraits, six rating-ball OBJ poses,29 home courts decoded offline from raw PPU dumps |261/264/271 are used by introductions.260 is legacy comparison data. Not screenshots, but full decoded BG/portrait content relies on captured native layout; source association and lifecycle timing remain unclosed. |
| 262 | ROM ball tile at file0D9C27 plus six literal palette colors → every live `draw_ball` | Tile source PASS; palette matches retained raw CGRAM but lacks extraction-time/native-publisher proof. |
| 263;272;273 | Legacy viewport,29 gameplay home viewports,29 full1184×416 panoramas decoded offline from ROM map plus raw/decompressed tiles | No normal gameplay consumer found for these flattened views.273 alone is57,135,128 bytes. Current game renders indexed284+279 instead. They are test/legacy baggage, not current rendering proof. |
| 265–268 | Introduction ARAM/DSP/state and captured DSP program → `nba_audio_play_player_intro_dsp` | No recorded song PCM; fixed downstream program, not live sequence translation. |
| 269–270 | Original fonts at ROM file148000/133B16 → introductions/lineups | Glyph bytes PASS; literal colors/host line and timing are separate presentation gaps. |
| 274–281 | Formation/play streams, CPU/shot/fatigue/jump/scratch tables and full court map from ROM → gameplay leaf functions/callers | Direct ROM bytes/graphs with selected structural/digest guards. They do not establish caller reachability or full game equivalence. |
| 282 | Raw basket BG map/CHR/CGRAM from tipoff dump | Packed legacy layer; current live goal renderer uses284 memory and256 object resource. No normal282 consumer found. |
| 283 |28 crowd tiles and palettes sampled from raw tipoff frames140/220/400 → `draw_animated_crowd` | Tile source is native memory. Runtime cycles three sampled states every8 C frames; complete original crowd-animation state machine/cadence is not established. |
| 284 |29 indexed gameplay VRAM/CGRAM bases, native court map/tiles and team palette patching → actual BG1/BG2/BG3 renderer | No emulator framebuffer. Native home selection/source caveats below; does not replay a rendered court panorama. |
| 285 |28 ROM BRR sources decoded to signed PCM in `NBGAUD1` → live gameplay mixer | Byte-source check PASS28/28 against native ARAM. Mixer/sequence parity FAIL as below. |

IDs12–15 are retired screenshot/title-WAV formats and absent from this pack;
75–79,158–159,190–191 are also absent. There is no external WAV-file read in
the production extractor. `.wav` use in extraction creates RIFF previews or
the EA composite from decoded BRR; this distinction matters.

## Court and palette caveats

The 2026-09-02 [court layout correction](court-logo-complaint-audit.md) resolves
the missing home selector in `build_gameplay_home_court_catalog`: Boston,
Milwaukee and Orlando use `$A0:8000` and destination `$B520`; the other exposed
teams use `$A0:BC26` and `$B4A0`. Both shared CHR blocks are independently
decompressed and checked against all 29 input captures. Boston/Milwaukee FB30
team streams are unsupported by the bounded decoder, not all-zero deltas;
fresh native uploads and visible-floor checks establish their final PPU input.

The normal runtime samples indexed tiles from284 and the selected map279/288, preserving
transparency and Mode1 layer priority; it does not blit asset273's panorama.
The live goal also uses these indexed memories and native object resources.
This is a meaningful architectural distinction from the prohibited intro
screenshot path. It does not remove the initializer's known reversed player
teams/uniform identities, or the separate camera/animation timing gaps.

The auditor searched the original ROM for the complete literal license,
default-pose palette and ball palette. None appears contiguously. That is
only a search result, not proof of invented art/colors. The ball palette does
match `.analysis/camera-source-20260823/tipoff_0140_cgram.bin` byte458 exactly.
The default-pose palette is diagnostic; live sprite colors are reconstructed
from ROM palette254, including the original skin overlays.

Normal pause/timeout UI in `nba_tipoff_render` is still drawn with host
rectangles and `nba_font` text/colors. Intro matchup/rating/lineup composition
also contains host layout/timing/line drawing. Debugger panels are outside
original-game artwork scope, but a normal pause screen is not a debugger and
must receive original UI assets/behavior before final visual acceptance.

## Audio findings

Asset7 is161,644 bytes, synthesized from ROM ranges12D9C5,12801C,11E03D and
11249B. Its16-kHz composite starts phrases at frame0,32,63,123 and pads to5.05s.
It is **not an emulator recording**, but the production path does not execute
the original command/sample/pitch/envelope sequence. The four individual
samples8–11 are previews; their presence does not make the composite native.

Title audio runs the SPC700 driver with assets83–86. However,86 timestamps
APU events by frame only. `nba_audio_play_title_spc` distributes multiple
events evenly among that frame's samples. That is an invented intra-frame
schedule, not exact native cycle timing. Its waveform fingerprints detect
large regressions; they do not establish sample-for-sample equivalence.

Setup asset93 contains114,059 recorded **register writes**, not PCM. The
runtime synthesizes150 seconds through the DSP-only path, bypassing the
SPC sequencer, then loops fixed sample positions2,053,956..4,048,365. Asset91's
289,435 APU writes are packed but not used by Setup playback. Documentation
explicitly says this bypass was adopted because SPC replay drifted. The0.97
onset-correlation threshold protects against two known bad outputs; it is not
exact proof of pitch, envelope, note timing, looping or option behavior.
Introduction268 similarly replays a captured downstream DSP program. Keep
these useful comparisons, but label finite-stream replay separately from
translated original sequence logic and dynamic command handling.

For gameplay, the auditor independently followed every original ARAM source
from DIR `$0200` to its end-marked BRR block and compared all bytes against the
extractor's ROM offsets: **28/28 match**. Evidence is
`.analysis/asset-ownership-20260830/gameplay-source-byte-check.json`;
native ARAM SHA-256
`88aae7c2de3bde18a225226758d1b10af1276b1b1080bce9ee48fbd2e6ecb33a`,
DSP SHA-256 `2a4fa3227ee2af4a54c602a83f8e9fe5d720adfecfd0817576b713e4fd89de0c`.
This rechecks byte provenance of the retained native bank; it does not
independently recapture that older run or prove its entire startup context.

The live mixer consumes offline decoded PCM, samples it with nearest-neighbor
16.16 cursors, applies fixed volume, and clips the sum. It has no native ADSR,
Gaussian interpolation, echo or native per-command voice allocation. Effect
voices rotate through host2–7; crowds are forced into host0/1 with fixed
pitches/loop starts. The native sample source and command-to-pitch vectors
are useful, but this implementation is an audio approximation.

`audio_gameplay_rng_next` deliberately uses a separate seed/state even though
its comment acknowledges original shared `$07F6`. It changes original RNG
consumption and later gameplay decisions; this is not merely platform audio
plumbing. Event bits are edge-latched, and all crowd-bit combinations collapse
to commands38/39 and sources14/15, while documented native families also
contain3A–3C/2F. Final command ordering and all source choices need native
caller-state comparisons. The whistle preview even logs ADSR8E/A0 although
this mixer never applies that envelope.

`test_gameplay_audio.py` checks original command-table bytes, a fixed packed
bank hash, bounds/loop constants, mixer startup and presence of several C
events over5,000 C frames. It does not compare that journey's audio commands
or samples to a synchronized original run. Its “live native event dispatch”
failure text and documentation's “natural1,200-frame command sequence” are
misleading. Controlled60-case command vectors cover isolated choices, not the
shared RNG, real scheduling, voice state or final sound.

## Required closure

The shared `Snes65816Decompressor.decompress` also needs a completion contract.
The auditor read its current loop: reaching10,000,000 steps or an unsupported
opcode simply leaves the loop, then returns the requested-size WRAM slice.
No successful native return is required. Thus correct output length, all-zero
data or a stable hash can conceal incomplete decompression. During the separate
intro repair, the integrator reports this failure for mode30 streams `$8D:FF5C`
and `$AA:B52F`, while mode46 `$9F:F121` reproduces11,776 native CHR bytes. The
per-stream result is that workstream's evidence; this audit independently
confirms the silent-success source defect. Extraction must require an observed
successful terminator and fail closed on unknown opcodes or exhaustion before
it can certify those resources. The intro worktree owns that correction.

Remove all direct and indirect PNG dependencies from production intro art;
derive the license bitmap and all palettes from documented native resources.
Require immutable raw-source manifests for every memory-derived pack input,
and fail extraction on mismatched native/ROM content instead of warning.
Preserve raw BRR sources and translate the actual sequence/command consumers,
or explicitly retain those systems as approximations until precise native
command/voice/sample comparisons pass. Verify original crowd animation,
pause/timeout graphics and introduction composition through normal callers.
Do not accept a blanket “ROM-derived assets/audio” PASS from file extensions,
the absence of RIFF in a music asset, or a stable pack/hash alone.
