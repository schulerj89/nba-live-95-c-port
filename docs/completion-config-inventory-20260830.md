# Configuration implementation and evidence inventory

Independent inspection of `52c2899` in `work/completion-auditor-20260830`.
This inventory is not approval of the unfinished integration or a complete-game
claim. The owner is repairing the CLI driver separately. No production files,
native fixtures, historical captures or baseline expectations were changed by
this inventory.

## Current evidence and field ownership

All 24 exposed values have implemented UI storage/commit logic. Current factory
Main is `[0,0,0,3]`, Rules `[0,0,1,0,0,0,0,0,1,1,1,0,0]`, Options
`[30,30,2,1,0,0,0]`. Main working `$16FB` is distinct from committed `$17AB`.
Selecting Style immediately updates active Rules; separately retained Custom
is not overwritten by selecting Arcade or Simulation. Session Custom and
configuration retention are implemented; disk/SRAM persistence is missing.

The stable menu/adjustment/canvas evidence accepted in the prior independent
audit remains bounded. Here the auditor personally reread the ROM instructions,
Ghidra and recomp for the input/default/preset paths, checked present C callers,
and reloaded **all ten** raw native configuration journeys corresponding to
the five committed fixture files. All actions, states, events and manifests
equal their compact fixture representations; the loader verified all **361**
source identities. This is fresh evidence-integrity verification, not fresh C
runtime parity. Six old manifests still have indirect success evidence and no
recorded numeric process exit; the four later runs record exit0. No exit was
invented. No `AGENTS.md` was present in this worktree or its ancestors.

In the table, **wired** means a production C reader exists, **bounded** means
the existing native helper/menu gate does not prove the complete configured
match journey, **missing** means the required consumer is absent. Every row
still needs normal gameplay observability where applicable and all original
entry/return/persistence interactions. Audio and visuals are not certified by
label or stored-value equality.

| UI / factory value | Stored native word | Current production consumer | Observable behavior / evidence still needed |
|---|---|---|---|
| Mode / Exhibition | Main0 `$17AB` | `nba_game.c:668` | Exhibition enters Team Select. Season, Playoffs, Load Series only print a route; **missing modes**. |
| Style / Arcade | Main1 `$17AD`; working `$16FD` | `nba_setup_screen.c:1288`; `nba_config_apply_style`, `$81:BFAA-$C00A` | Immediate presets and separate Custom are **wired, bounded menu-verified**. Full per-preset match effects remain unverified. |
| Level / Rookie | Main2 `$17AF` | `nba_tipoff.c:3052,3809,3858` | Contact/shot readers **wired, bounded**; each level's natural human/CPU effects unverified. |
| Quarter / 12 minutes | Main3 `$17B1` | `nba_session.c:59-80`, `nba_tipoff.c:9016`; `$86:DBDC/$DD2D` | Regulation/OT tables **wired**, lifecycle C tests. Whole native start/scheduler still mismatched. |
| Defensive Fouls / 0 | Rules0 `$17D1` | `nba_tipoff.c:2385,3053`; `$86:C4FE-$C6AC` | Probability threshold **wired, bounded**. Menu-to-match OFF/threshold contact outcomes missing verification. |
| Offensive Fouls / 0 | Rules1 `$17D3` | `nba_tipoff.c:2386`; `$86:C4FE-$C6AC` | **Wired, bounded**; same configured-contact gap. |
| Out Of Bounds / ON | Rules2 `$17D5` | `nba_tipoff.c:5035` | Production gate **wired, bounded native OOB vectors**; configured OFF journey unverified. |
| Backcourt / OFF | Rules3 `$17D7` | No C reader; native `$87:902E-$9072` | **Missing** rule consumer. |
| Traveling / OFF | Rules4 `$17D9` | No C reader; native `$85:EFB0`, under `$85:EF3A-$EFEC` | **Missing** grounded/moving human ballowner violation5 path; depends on human ownership. |
| Goaltending / OFF | Rules5 `$17DB` | `nba_tipoff.c:3180,5086` | Contact/violation readers **wired, bounded**; configured natural effect unverified. |
| 3 In The Key / OFF | Rules6 `$17DD` | No C reader; native `$85:9685` | **Missing** rule-gated actor commit behavior. |
| Foul Out / OFF | Rules7 `$17DF` | `nba_tipoff.c:2388,3065`; `$86:C493-$C4FD`, `$83:ECB0-$ED46` | Bookkeeping/substitution helper **wired, bounded**; complete user substitution journey unverified. |
| Shot Clock / ON | Rules8 `$17E1` | `nba_tipoff.c:3808,8974` | Shot policy/clock **wired, bounded**; OFF event suppression through menus unverified. |
| Inbound Clock / ON | Rules9 `$17E3` | No C reader; native `$87:9A84-$9ACA` | **Missing** OFF-aware inbound expiry. |
| Half Court Clock / ON | Rules10 `$17E5` | No C reader; native `$87:9AD6-$9B08` | **Missing** conditional violation. |
| Fatigue / OFF | Rules11 `$17E7` | `nba_tipoff.c:9015`; native `$87:8EF3` | Stamina helper before actor sweep **wired, bounded**; duration/OFF/timeout/substitution journey unverified. |
| Injuries / OFF | Rules12 `$17E9` | No C reader; native `$86:C193` | **Missing** injury and availability propagation. |
| Music Volume / 30 | Options0 `$17B5` | `nba_game.c:654`; `nba_audio.c:884` | Setup-only host gain **approximation**; native `min(value*3,127)`, current `value*65535/45`. Gameplay consumer missing. |
| SFX Volume / 30 | Options1 `$17B7` | `nba_game.c:657`; `nba_audio.c:599,878` | Setup-only DSP gain **approximation**, current `value*64/30`; native gain contract unresolved downstream. Gameplay setting ignored. |
| Music Mode / Stereo2 | Options2 `$17B9` | No runtime audio reader; native `$87:8CF3/$8D12/$8D38/$8D6C/$8D85` | UI storage only; **missing** OFF/Mono/Stereo behavior. |
| Crowd Sound / ON | Options3 `$17BB` | No runtime reader | UI storage only; **missing** crowd gate. |
| Slow Motion Dunks / No0 | Options4 `$17BD` | No runtime reader; native `$87:8E5B-$8E7C` | UI storage only; **missing** scheduler effect. |
| Shot Control / Player0 | Options5 `$17BF` | `nba_tipoff.c:3859` | Shot-assistance field **wired, bounded**; human ownership incomplete. `$17C3` is distinct, not this option. |
| CPU Assistance / No0 | Options6 `$17C1` | `nba_tipoff.c:4820` | Late-game `$09C0` helper **wired, bounded**; configured natural effect unverified. |

Runtime readers above were searched throughout `src/`, not inferred from UI
names. Several `nba_tipoff.c` config assignments belong only to selftests and
were excluded as production evidence. Existing audio track playback also uses
recorded command schedules (`nba_audio.c:980+`); BRR source bytes alone do not
make that path an acceptable original sequence/command implementation.

## Concrete legacy harness migration

The candidate native producer reads `input.held`. `src/main.c:672-854` currently
sets only `input.pressed`. Fix the driver, keep one full button word per frame,
derive pressed/released from consecutive held words, and explicitly release
between separate tap commands. Do not weaken production native input to treat
pressed-only data as held. Adjacent equal words with no empty frame are a hold,
not a second tap. Preserve combined words and native delay/fast state.

For a **configured Simulation/3-minute** scenario from fresh Main row0, use real
menu presses: Down, Right (Style0 to1), Down, Down, Right (Quarter3 to0), Up,
Up, Up. Read working `[0,1,0,0]` before commit and committed values at the
actual submenu/match boundary. Native reference scheduling in
`capture_setup_transition_exact.ps1:78-79` is actions at400,460,520,580,640,700,
760,820 and a label rebase at920 to370. This is not cold-boot frame parity.
C may use a declared input-only alignment, but must establish the actual
dispatch/scroll phase rather than assume the historical717 idle survives the
added actions. Never mutate a native oracle from C output.

| Current test | Assumption / migration |
|---|---|
| `test_setup_rules_reveal.py:119,223` | Requires native `[0,1,0,0]`, C command omits configuration. Explicit real-menu profile and independently fixed alignment required. |
| `test_setup_rules_settled.py:122` | Old bar/default/row snapshots require Simulation rules. Add the configured profile before submenu and recompute only C input dispatch alignment. |
| `test_setup_rules_return.py:27,134` | Native unchanged Simulation and changed row2 Custom assume45/45 and all ON. Explicit profile required, retain native arrays. |
| `test_setup_rules_reentry.py:29-35,124` | Second visit depends on retained Custom row2 OFF then ON and quarter0. Explicit first-visit profile; never reapply it on reentry. Scheduler phase remains a separate production failure. |
| `test_setup_transition.py:494,677-763` | Main state, left/right/wrap, persisted and reentered assertions hardcode `[0,1,0,0]`. Either classify them configured scenarios and drive profile or use independently captured factory/Main-v4 expectations. Working telemetry must read `working_main`, not committed Main. `--setup-reenter` is controlled component reset, not disk persistence. |
| `test_setup_transition.py:783-958` | Old settled/transition pixel hashes and fixed frame/scroll checkpoints depend on configured state and dispatch timing. Preserve independent native expectations; separate C-only hashes. |
| `test_setup_transition.py:1055-1067` | Right on default foul slider is expected to clamp45. Under factory Arcade it changes0 to1. Use explicit Simulation setup for this endpoint case. |
| `test_setup_transition.py:1090-1098` | **Wrong native contract:** Down13 in Rules must clamp row12, not wrap0. Existing raw Rules journeys prove the clamp. Options still wraps. Correct this assertion from that evidence. |
| `test_setup_transition.py:1087` | Asserts host gain30→`$40`,31→`$42`; keep classified as approximation regression until native gain/allocator is implemented. Do not cite it as native audio parity. |
| `test_setup_transition.py:768-778` | Season/Playoffs/Load Series only assert printed route/action. This cannot credit implemented modes; add real scene/lifecycle checks when those modes exist. |
| `test_cpu_gameplay.py`, direct `--tipoff-only` tests | C regression, often explicitly forced clock43200. Factory defaults now also alter rules/fatigue/contact/RNG. Direct-entry probes may declare injected config, but cannot claim real-menu journeys. Preserve natural journey gates separately; do not update goldens merely to hide unequal prestates. |

`run_setup_config_checks.ps1` must follow a fresh build of all current
production objects: `build_vector_probe.ps1` checks existence and can otherwise
link stale objects. Its stable/adjustment gates drive normal C menu callbacks
but start directly at Setup and exclude transitions/intro, multi-controller
host wiring, runtime effects and C disk reload.

## Reverified source identities

Full paths, sizes and SHA256 values plus the raw/compact replay report are
retained at this worktree's `build/config-inventory-audit/source-and-raw-audit.json`.
The original ROM remains1,572,864 bytes and SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

| Source | SHA256 |
|---|---|
| `.analysis/setup-config-native-20260830/preparation/complete-input/setup_config_bank81.txt` | `337c4600ceb87189ff0b661c1575e486d8cc6c84372648df3ca4a382d4241b20` |
| `.analysis/setup-config-native-20260830/reference/bank81-with-repeat.c` | `222733626ca943226f8ba604d27577eceb9378003cb223c0a78f46ba417fb7ec` |
| `.analysis/setup-config-native-20260830/reference/bank82.c` | `ecf7532ca8fe676a8636307a4d168f9d00e79d387dc040270abb8430d13f778b` |
| `src/nba_session.c` | `6df0196961cf5d13bd2febb9ffc96c9776a82f3e972b94aba990d8ba770cbcf2` |
| `src/nba_menu_input.c` | `3aff5eb5f54da54e7b77dfd31b1c08f51b042b3d3ca3c8d20af24ec2826fc1ae` |
| `src/nba_setup_screen.c` | `23072ed13bc4a79bb805b200503fb82de69cd0af94487a6dd000dd90273436d9` |
| `src/main.c` before owner driver patch | `454b19955ecad2b8775632145d4aec052646908a196b39ca22d00492646098a3` |

ROM byte spot checks included `$81:AB58` input producer (`A2 08 00 9B`),
`$81:BFC6` Simulation (`A9 2D 00` then threshold stores), `$81:C1BE` factory
volumes/quarter and `$81:C21D` Custom initialization. Ghidra/recomp M/X widths
agree:16-bit immediate default words, SEP20 for Custom threshold bytes,
REP20 for bitmap`07FC`. Those are bounded routine checks, not full instruction
or game closure. Detailed full native consumer retranslation remains assigned
to implementation milestones; source references inherited from the prior
ownership audit are not new whole-routine equivalence claims here.

Independent acceptance of the owner's input/config integration is pending a
frozen patch, fresh same-version build, exact replay and first-failure review.
