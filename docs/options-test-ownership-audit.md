# Rules, options and evidence ownership audit

Audit of `2723af610aab0ec63263a6449fa6a161a155f974`, 2026-08-30. The
working tree was clean when inspected. This is a source/evidence audit, not
a declaration that any newly listed option has passed a real-match test.

## Sources inspected

- Portable state: `include/nba_session.h`, `src/nba_session.c`,
  `src/nba_setup_screen.c`, `src/nba_game.c`, `src/nba_tipoff.c`,
  `src/nba_audio.c`, and gameplay AI/foul/shot modules.
- ROM: `F:/Games/SNES/NBA Live 95 (USA).sfc`. Label pointer table at ROM file
  offset `$00CDDD` directly establishes the thirteen Rules rows; Fatigue is
  row 11, Injuries row 12. Run Speed is a following, unexposed string, not
  a fourteenth menu row. Options labels begin at `$010A02`.
- Ghidra: `.analysis/setup_capture/setup_bank81_listing.txt`,
  `setup_bank82_listing.txt`; `.analysis/gameplay85-closure-ghidra/`;
  `.analysis/pending-owner-inbound-recount-20260827/shot_state_bank86.txt`;
  `.analysis/actor-commit-clamp-audit-20260829/actor_commit_clamp_bank85.txt`.
- Recomp: `C:/Users/joshs/Projects/NBA-Live-95-Recomp/generated/` is a
  startup-oriented generated set, not the complete gameplay reference.
  `.analysis/owner-flow-recomp-20260827/generated/bank86_v2.c` contains
  `L_F43A_M0X0` / `L_F4E6_M0X0`; the latter restores `$AE,$B0,$AA,$AC`
  from the stack before the raw arrival comparisons. Dedicated shot-state
  and shot-completion generated banks also exist under `.analysis/`.
- Native witnesses: `.analysis/inbound-continuation-native-20260829/`,
  `tests/fixtures/inbound-*.json`; capture driver
  `tools/mesen_func_vectors.lua`; normalizers/verifiers/probes in `tools/`.
- Test/status claims: `STATUS.md`, `docs/differential-testing.md`,
  `docs/inbound-motion-differential.md`, `docs/game-setup-screen.md`,
  `tools/test_cpu_gameplay.py`, `tools/test_setup_transition.py`,
  `tools/gameplay100_closure_probe.c`, and relevant Git diffs.

No repository/ancestor AGENTS.md was found; the inspected
`C:/Users/joshs/.codex/AGENTS.md` was empty.

## Configuration mapping

All rows have implemented UI storage and commit behavior unless an exception
is identified below. `main[n]`, `rule[n]`, `option[n]` abbreviate
`session.config.main_values`, `.rules`, `.options`. The committed native main
block is `$17AB`; `$16FB` is reused working storage, not a persistent unique
field for each screen. Rules commit to `$17D1`, Options to `$17B5`.

“Bounded” below means a helper has native vectors and a production reader
exists. It does **not** mean a complete menu-to-match option journey was
independently verified. No such complete per-value matrix exists today.

| UI item/current C default | Stored/native field | Native consumer / C consumer | Observable behavior and actual evidence/gap |
|---|---|---|---|
| Mode / Exhibition | main[0], `$17AB` | `$81:BD36` commit; `nba_game.c` confirm dispatch | Exhibition enters Team Select. Season/Playoffs/Load Series merely print route; no scene is entered. UI tests do not complete those modes. |
| Style / Simulation | main[1], `$17AD` | `$81:D491` marks Custom; native preset path needs complete translation | Transition checkpoint now marks Custom on Rules adjustments, including clamps; bounded native-prestate real-caller gate passes. Arcade/Simulation presets and separate saved Custom remain missing. |
| Level / Rookie | main[2], `$17AF` | shot policy/launch, contact gates; `nba_tipoff.c` | Several real readers and bounded vectors. Full difficulty effects, real input and each configured match not verified. |
| Quarter / 3 minutes | main[3], `$17B1` | `$86:DBDC/$DD2D`, tables `$E38A/$E392`; `nba_match_period_clock`, fatigue | Regulation 10800/18000/28800/43200 ticks, overtime 7200/10800/14400/18000; helper/lifecycle tests exist. Native whole-start baseline still has different clock. |
| Defensive Fouls / 45 | rule[0], `$17D1` | `$86:C4FE-$C6AC`, pose/contact gate; foul classifier | Probability threshold consumed. Bounded vectors and default endurance; OFF/all bar values through menu and natural match not established. |
| Offensive Fouls / 45 | rule[1], `$17D3` | `$86:C4FE-$C6AC`; foul classifier | Probability threshold consumed; same evidence limits. |
| Out Of Bounds / On | rule[2], `$17D5` | violation parent; `cpu_process_pending_event` | Production rule gate and native OOB vectors. Configured OFF normal-match path still needs proof. |
| Backcourt / On | rule[3], `$17D7` | `$87:902E-$9072` | No C config reader. Missing rule consumer. |
| Traveling / On | rule[4], `$17D9` | `$85:EFB0` in native human dispatch `$85:EF3A-$EFEC` | No C config reader. Native requires actual grounded/moving ballowner and its action/lifecycle gates, then publishes violation5 and offending actor. Current forced-CPU path does not reach this human consumer; detached helper coverage would not prove wiring. |
| Goaltending / On | rule[5], `$17DB` | native contact/violation paths; `nba_tipoff.c` | Reader exists in both contact and violation stages. Bounded proof does not cover full configured journey. |
| 3 In The Key / On | rule[6], `$17DD` | `$85:9685` during actor commit | No C config reader. Missing production rule gating. |
| Foul Out / On | rule[7], `$17DF` | `$86:C493-$C4FD`, `$83:ECB0-$ED46`; bookkeeping and substitution | Config flag reaches foul-out bookkeeping; controlled helper/runtime tests exist. User substitution menu and native visual journey remain separate. |
| Shot Clock / On | rule[8], `$17E1` | shot clock/policy; `nba_shot_clock_step`, CPU shot policy | Real readers; helper tests/default endurance. OFF event suppression through menu not established. |
| Inbound Clock / On | rule[9], `$17E3` | `$87:9A84-$9ACA` | No C config reader. Existing timeout expiry cannot honor OFF. |
| Half Court Clock / On | rule[10], `$17E5` | `$87:9AD6-$9B08` | No C config reader. Missing conditional violation. |
| Fatigue / On | rule[11], `$17E7` | `$87:8EF3` caller / stamina helpers; `nba_shot_fatigue_step` | Wired before actor sweep. Bounded stamina proof; each duration/OFF/timeout/substitution journey still required. |
| Injuries / On | rule[12], `$17E9` | `$86:C193` contact branch | No C config reader. Missing injury behavior/availability propagation. |
| Music Volume / 30 | option[0], `$17B5` | `$82:8DDC -> $87:8C2D`; `nba_audio_set_setup_music_volume` | Immediate Setup gain only. Does not reach gameplay mixer. Menu audio tests exist; real-match effect missing. |
| SFX Volume / 30 | option[1], `$17B7` | same native gain dispatcher; `nba_audio_set_setup_sfx_volume` | Immediate Setup SFX gain only; gameplay mixer ignores the setting. |
| Music Mode / Stereo (2) | option[2], `$17B9` | `$87:8CF3/$8D12/$8D38/$8D6C/$8D85` | C changes glyph canvas only. Off/Mono/Stereo audio consumers missing. |
| Crowd Sound / On | option[3], `$17BB` | native menu serialization `$81:C159`; complete gameplay audio gate needs mapping | C changes glyph only. Crowd bed/event voices are unconditional. |
| Slow Motion Dunks / No (0) | option[4], `$17BD` | `$87:8E5B-$8E7C` scheduler accumulator | C changes glyph only; no scheduler reader. |
| Shot Control / Player (0) | option[5], `$17BF` | shot assistance branch; `NbaShotLaunchInput.shot_assistance_17bf` | Field reaches shot launch, but branch needs human ownership. `shot_control_17c3` is a distinct native field, remains zero in current adapter. Do not conflate them. |
| CPU Assistance / No (0) | option[6], `$17C1` | `$09C0` late-game trailing-team helper; `nba_tipoff.c` | Reader/bounded helper exists. Real configured late-game effect not demonstrated. |

Session memory survives Setup reinitialization. It is neither disk nor SRAM
save persistence. No configuration save/load implementation was found.
Default tables are host constants. A fresh native initialization comparison
now proves the startup default mismatch described below; this was not silently
changed while establishing a matched transition baseline.
Both submenu discrete cycles and clamps have C UI regressions; they do not
prove runtime use. `gameplay100_closure_probe` changes only rule[0]/option[0],
then checks stored values and a C digest; it does not demonstrate all options.

### Fresh native cold-boot defaults

Independently decoded `wram_before_open.bin` from
`.analysis/transition-ownership-20260830/setup_rules_canonical_v2/` after the
gameplay auditor established its private portable Mesen home and initially
empty save directory. The 128-KiB WRAM SHA-256 is
`c39e174f2b62a016f59ae001dbbe2b1a72e0abfe5d99c14ef6852670fe72506e`.
Both `$16FB` working and `$17AB` committed main fields are `[0,0,0,3]`:
Exhibition, **Arcade**, Rookie, **12 minutes**. Rules at `$17D1` are
`[0,0,1,0,0,0,0,0,1,1,1,0,0]`; Options at `$17B5` are
`[30,30,2,1,0,0,0]`. Thus Options defaults match; C main Style/Quarter and
its Simulation-like Rules defaults do not match a fresh native boot.

Earlier captures using `--preferences.saveDataFolder=...` were not isolated:
this installed Mesen does not implement that string command-line setting and
could read the user's existing SRAM. A prior Custom/12-minute observation was
therefore not evidence of the cold-boot default. Selecting Simulation/3-minute
values through natural UI input is a valid *normalized test configuration*,
not the original default and not a reason to rewrite the raw baseline.

## Tests and earlier claims

1. Current status correctly distinguishes coverage: 28,643 captured address
   positions documented; 11,529 evidence-eligible (40.25% of that capture
   union). Mixed gap-coalesced interval addresses are not instruction starts.
   The 11,526/60,346 conservative decoded-start count (19.10%) excludes
   undiscovered/decode-ambiguous code and is not a game-completion fraction.
   Feature weights are engineering estimates. Former broad bank credit was
   explicitly revoked; it must not return through new “host equivalent” rows.
2. The differential runtime gate currently fails initialization. Its 449-word
   projection omits full team/stat/controller/PPU/APU state. Native first actor
   sweep and C first actor sweep occur at different video frames. A projected
   match would still not be whole-state or whole-game equivalence. The runner
   correctly refuses an overall parity PASS even for an equal projection.
3. `test_cpu_gameplay.py` hashes and endurance are C regressions. They protect
   reviewed behavior, not ROM frame parity. The recent comments say so. Hash
   updates cite visual reviews, but those reviews do not prove native
   trajectory; fresh paired frames are still necessary at transition gates.
4. `differential_observer_probe` compares observed C with unobserved C. It is
   useful evidence that instrumentation is nonmutating, not ROM equivalence.
5. Generic native vector capture drives menus by inputs but also writes Mode
   and (when requested) transient controller/ownership words. These are
   controlled setup interventions. Its CPU-only clear happens before the
   actual Player Setup consumer can reassign controllers. Existing “natural
   call” fixtures mean native routine execution, not an untouched natural
   boot/menu journey or verified CPU-only context. New captures must record
   interventions and final controller assignments explicitly.
6. Inbound arrival normalizer hardcodes raw population 500 and launch count 2;
   this is metadata manufactured by the normalizer, not validated provenance.
   It must derive and validate these counts. Empty or truncated input must not
   be describable as a complete capture.
7. Inbound motion verifier discards the fourth probe token (`split()[:3]`).
   Fixture owns velocity X/Y and boost only, while its document claims
   direction. That direction claim is unsupported. Require complete output
   parsing and compare direction against a native internal boundary before
   expanding the proven scope. No new expected direction may be generated
   from the implementation under test.
8. Native vector tests do not by themselves prove real-caller reachability.
   Controlled vectors are valuable edge tests but must retain their provenance
   and complement normal menu/gameplay journeys.

## Inbound compensation finding

Commit `a893dbb` widened first-arrival telemetry to ±80 pixels, claiming
same-dispatch velocity compensation made exact checking impossible. Current
HEAD `2723af6` already removed that envelope and restored raw `[-9,+8]`.
Independent inspection confirms that correction: `$86:F4E6-$F4F0` restores
the original DP target after `$85:B3C9`, and `$86:F4F2-$F51D` compares that
raw target with actor integer words. Compensation affects steering only.

Missing internal evidence should still be captured: routine entry, compensated
target immediately before B3C9, velocity after B3C9, restored target at F4F2,
arrival/rejection branch, and post-readiness state. End-of-frame velocity may
already be zeroed by F563/F566. Capturing those points resolves the actual
dependency without any tolerance, and distinguishes helper proof from the
normal production-caller proof. New artifacts/results will be appended here;
the existing exact raw arrival guard is preserved.

## Dependency order

1. Finish and independently audit transition capture/order fixes first.
2. Establish exact saved configuration/default/preset semantics, then expose
   the above table as a machine-checked menu-to-match test inventory.
3. Connect missing rule gates and audio/scheduler consumers with bounded native
   tests, then normal matched-input journeys for OFF and each nondefault value.
4. Complete human controller ownership before crediting human shot-control
   effects; complete injury/substitution lifecycle before crediting injuries.
5. Align initialization/scheduler/RNG and expand full-state logical checkpoints.
6. Release only after configured real-match journeys, independent parity and
   visual/audio review pass. No percentage here licenses a completion claim.

## Fresh inbound internal evidence, 2026-08-30

`tools/capture_inbound_internal.ps1` and `tools/mesen_inbound_internal.lua`
record 500 complete original-ROM dispatches in
`.analysis/inbound-internal-ownership-20260830/`. `run.json` binds ROM, Mesen,
scripts and raw trace hashes, requested population and setup intervention.
The native default controller context includes both `0000` and `FFFF`; it is
explicitly **not** a CPU-only capture. The generic driver writes Exhibition
Mode before Start; no runtime state, CPU register or ROM byte is injected.
This inbound run used the installed/global Mesen home, not the subsequently
validated private portable configuration. It does not claim cold-boot default
or whole-journey initialization equivalence. Its internal snapshots expose
the actual helper inputs and preserve the validity of the bounded comparisons.

The direct internal verifier compares F43A input, compensated target before
B3C9, direction passed to A82C, same-dispatch post-motion velocity, restored
raw target at F4F2, arrival/rejection and readiness outputs. All **500**
motion calls compare all four velocity-X/velocity-Y/boost/direction outputs;
all **389** prepared arrival projections match. There are **111** rejected
arrival boxes, **470** calls with internal velocity changes, and both signs
of velocity. Every restored target equals the raw target. No tolerance or
frame-offset search is used. Report: `report.json` in the capture directory.

ROM SHA-256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Raw trace SHA-256:
`221ccff5f027e98f522cbd136cc901f660627aa407abcee76f20809483b2beed`.

Existing 111-call motion and 387-call arrival fixtures still pass unchanged.
Their parsers now reject extra/missing/malformed words and rows; the historical
motion fixture explicitly does not own direction. The arrival normalizer
derives populations from raw records and preserves a source hash. Regeneration
produced the same 387 expectation payloads. Nine synthetic adversarial tests in
`tools/test_inbound_verifier_integrity.py` pass; they test protocol integrity,
not the ROM. Full C runtime pre-call observation and normal-journey schedule
equivalence remain separate from the exact production-helper comparisons.

The permanent `tests/fixtures/inbound-internal-witnesses.json` is a lossless
683,030-byte encoding of all **3,889** native snapshots, with one shared list
of 41 field names. It retains every captured field at every boundary, along
with source ROM/Mesen/capture-script/driver/trace hashes. Its decoded records
were checked field-for-field against the original raw JSONL. The normalizer
refuses to overwrite an existing fixture and never invokes C to manufacture
expected values. `verify_inbound_internal.py --vectors` replays it through the
two production helpers; native controller words and setup intervention remain
visible in the report. The integrity suite also mutates each of the four
motion and ten arrival outputs, proving every represented output is checked.
