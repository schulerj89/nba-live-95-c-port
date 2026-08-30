# NBA Live '95 C-port parity gap report

Reaudited 2026-08-29. The repository is **not 100% ported and is not in
whole-game behavioral parity with the SNES ROM**. The former 100% figure
measured addresses in a retained capture set and also credited broad
host-equivalent ledger ranges. It was never a percentage of the ROM, the
retail feature set, or matching gameplay trajectories.

## Corrected evidence-accounting snapshot

The strict ledger audit removes aggregate host-equivalent entries from address
credit and validates every composite range independently. The fresh four-pass
Ghidra census and regenerated progress report now give:

| measurement | verified | denominator | result |
|---|---:|---:|---:|
| Retained captured-address positions | 11,529 | 28,643 | 40.25% |
| Conservative full-ROM decoded instruction starts | 11,526 | 60,346 | 19.10% |

The numerators differ because the first report counts captured address
positions while the second counts only Ghidra-decoded instruction starts that
are both observed and inside an evidence-eligible ledger boundary. They must
not be interchanged. `docs/progress.md` and
`docs/full-rom-instruction-census.md` are the generated authorities.
The current ledger contains 233 rows, of which 207 are eligible for address
coverage.

The full-ROM denominator is itself a conservative lower bound seeded from
execution, verified boundaries, provenance, recomp discoveries, and SNES
vectors. Undecoded ROM bytes may be data or undiscovered code. Separately,
`docs/feature-capture-matrix.md` reports a weighted engineering estimate for
retail features; that planning score is not native code coverage.

## Why the old 100% claim fails

- Captured-address coverage only asks whether an address appeared in retained
  traces. Uncaptured game modes, human paths, and rare branches are outside its
  denominator.
- Whole-bank and wide `host equivalent` entries describe portable final-output
  boundaries. They are useful architecture evidence, but they are not exact
  native instruction replay and therefore receive no address credit.
- Ghidra and recomp establish ownership and control-flow hypotheses. A range is
  not behaviorally verified until bounded native inputs and outputs replay
  through the C implementation with zero represented-output mismatches.
- Long CPU simulations and render hashes protect integration, but cannot prove
  the same initial state, update order, RNG history, or unseen branch results.
- The refreshed strict CPU-only differential starts with 62 differences among
  449 projected words and zero matching checkpoints; the default one-human
  launch has 63. The first actor sweep is at native relative frame 25 versus C
  frame 2. Later trajectory differences are not fairly attributable until that
  baseline is aligned.

The latest run is `.analysis/differential-native-edges-final-cpu`: 51 native
and 51 C checkpoints, 25 actual actor sweeps, and all ten controllers `FFFF`.
It retains the baseline failure; no snapshot import, rebasing or field masking
was used to turn it green.

## Subsystem gaps

“Unknown” means there is no defensible full-subsystem instruction denominator.
Named counts describe bounded scopes; they are not whole-feature totals.
A zero mismatch applies only to captured outputs and represented branches.
Detailed new evidence and remaining caveats are in
[native-edge-parity.md](native-edge-parity.md).

| Subsystem | ROM/Ghidra ranges | Recomp functions | Total/pending instructions | First divergence | Implementation status | Differential coverage | Runtime/endurance coverage | Visual/audio coverage | Confidence | Remaining gaps |
|---|---|---|---|---|---|---|---|---|---|---|
| Initialization/scheduler | $86:E056-E0AB; $87:8EFB-8FA9 | Bank86 E056 prefix; Bank87 scheduler extraction | 30/0 in ball-init prefix; wider total/pending unknown | Baseline62 CPU or63 human word differences; first sweep native25/C2 | Ball prefix exact; equivalent game initialization unfinished | Two full-WRAM prefix fixtures;449-word strict trajectory fails baseline | Observer checks1,000 real actor sweeps without changing gameplay | Initial court/camera captures only | High prefix; low whole-start parity | Align state, ownership, RNG and logical-frame boundaries before locating a downstream first divergent routine |
| Human free throws | $87:9CBF-A045; launch $86:B991+ | HumanFreeThrowScene_M0X0; HumanAimOscillator_M0X0 | Oscillator precisely credited; dispatcher/common-launch pending unknown | Human B/Y aim branches bypassed by CPU-only handling | Two-axis helper and dormant scene adapter implemented | Seven native state/aim witnesses from1,556-call genuine-input corpus | Helper and binding tests; ordinary controller ownership not connected | Cursor-art writes and complete common-launch timing excluded | High represented aim words; low playable flow | Player Setup ownership, natural foul-to-stripe inputs, artwork and common-launch side effects |
| Human offense/defense | $84:E2AC-E432; $86:F520-F54E | Wider Bank84 indirect dispatch unresolved; focused Bank86 inbound body | 136 captured Bank84 address positions, not an instruction census; pending unknown | Ordinary assignments remain CPU-only | Isolated direction/aim helpers, no complete human game | All16 inbound direction nibbles; no complete input-movie parity | CPU tests do not cover human gameplay | Human overlays/action/audio sequences incomplete | Low | Movement, pass/shot/defense, switching and controller-loss paths |
| Owned/free ball driver | $85:9A37-A7C7; $87:B649-B67B | bank_85_9A24_M0X0; pose composers | Narrow wrapper credit; full core pending unknown | Host ball label/stale handler selected physics instead of signed $093E; actor fractions copied | Signed ownership dispatch, integer-only projection and twice-per-pass low-owned substeps implemented | 323 complete owned cases plus1 partial;431 complete replays plus1 partial; ownerless/rim fixtures separate | Stale-ATTACHED binding guard and sustained CPU play | ROM-derived attachment assets; no synchronous audio claim | High captured words; partial event timing | Call181 native $13E7=0 versus isolated C=1 crosses a frame; interrupt consumption inferred, not proven; owned rim cases unexhausted |
| Actor motion/edges | $85:96B5-9A13; data $9A14-9A23 | bank_85_963D_M0X0 | 224 distinct PCs in controlled fixture;66 newly witnessed edge starts; full pending unknown | Stationary axes clamped, fractions erased, mode8/timer60/edge codes omitted | Exact rectangle/diagonal helper and ordinary production binding | 56 controlled cases plus1,490 older calls;19 projected outputs match | Native bounds run once on common-commit paths | Changed C visual anchors reviewed separately; event40 producer excluded | High arithmetic; medium callers | Special-mode compatibility integration, landing children and $98F4 shared RNG/event side effects |
| Inbound/formations | $86:F43A-F668; $85:AD6B-AF5B; $85:C37D-C65B | bank_85_AD6B_M0X0; FormationRoute_M0X0; InboundSideGate_M0X0 | Bounded helper scopes; complete caller total/pending unknown | Arrival used a steering-only compensated target; rounded source X and hardcoded halftime anchors | Raw [-9,+8] arrival, integer targets, live context anchors and C602 play request bound | 387 arrival witnesses; selector witnesses;40 side-gate cases;96 formation plus10 override cases | 63,800-frame run progresses through halftime/OT; rare canceled-transfer coverage is tested separately | C runtime frames reviewed; native transition pixel/audio timing incomplete | High helpers; medium orchestration | Rare callers and equivalent whole-game trajectory; moving actors still cannot reach arbitrary raw X403 targets |
| Contact/fouls/violations | $86:CFA0-CFDE/D34A-D3C5; $87:92A5-949E | ViolationOobParent_M0X0; Bank86 classifier/acquisition bodies | Full subsystem total/pending unknown | Early-play collision may rewrite $0954; owned OOB incorrectly used loose-ball velocity gating | Conditional carrier gate, owned/free OOB split, first-axis priority and both-axis stop implemented | 46 controlled OOB plus10 original parent cases; contact/foul/consumer fixtures separate | Fouls, acquisition and inbound integration/endurance guards | Whistle command witnesses; complete event cadence unaligned | High represented parent words; medium caller graph | Bonus/penalty orchestration, rare violations, shooter replacement and bench/stat side effects |
| CPU AI/action callers | $85:A82C-C0F5; $86:E39A-F961 | Named planner/selector and actor extraction bodies | Full total/pending unknown | Whole-run differences already present at baseline; later first routine unresolved | Many exact selectors, assignments, poses and actions; compatibility callers remain | Independent helper fixtures; C-only digests explicitly separate | Multi-team endurance and natural action/animation probes | ROM resources/component compositor tests | Medium | RNG order, full action-prefix ordering, unobserved decisions and branch frequencies |
| Clock/period/halftime/OT/final | $86:DBDC-DBE5/DD2D-DD44; $87:97A0-985C; scene entries $87:C2F3/CC36/D2AE, $83:FA91 | Clock/expiry extraction; scene/persistence graph incomplete | Eight table values plus bounded expiry paths; full pending unknown | Scene waits were approximated; raster children still incomplete | Native tables, expiry gates, anchor reversal and measured waits; structural final panel | Four period-expiry witnesses; natural whole-match equivalence absent | Short-clock handoffs and long regulation/OT run | Waits1187/1547/1367/1756; scene pixels not fixture-backed | High tables; medium logic; low raster | Break/final PPU composition, exits, stats and mode-specific side effects |
| Timeout/substitution menus | $86:8300-857B; $83:ECB0-ED46 | Timeout/resume and substitution excerpts | Nine timeout-confirmation starts not fully caller-verified; wider pending unknown | Only selected pause/resume/substitution continuations proven | Pause, resume and automatic foul-out handling; full menus unfinished | Bounded state/table witnesses, not every menu path | Freeze/resume and lineup binding guards | No complete native menu-frame/audio sequence | Medium-low | Every menu option, disabled choices, lineup/bench UI and resume variants |
| Season/playoffs/persistence | Routing below $87:985C; indirect Bank83/87 graph unresolved | Verified recomp discovery incomplete beyond indirect dispatch | Unknown/unknown; no invented whole-mode denominator | Exhibition path lacks persistent retail modes | Unfinished | No equivalent season/playoff/save/load trajectory | Exhibition tests do not cover these modes | Screens/mode transitions incomplete | Low | Non-exhibition flow, bracket/results, persistence and Load Series |
| Camera/PPU/OAM/assets | $85:8B98-8BBE/9192-9351; $87:A357-A5FA/A9D0-A9E2; Bank80 services | Camera/draw extraction; host-visible PPU boundary | 99/0 scoped camera starts; five unobserved culling starts excluded; wider pending unknown | Baseline actor/camera inputs differ; moving-frame first divergence unresolved | Native camera/projection and ROM-derived indexed layers; host renderer | 1,133 camera calls;480 presentation calls; raw sprite/pixel and compositor oracles | 16,000 updates/812 viewports across29 courts | C hashes plus native component oracles, not whole-motion pixel parity | High components; medium final frames | Crowd CHR cadence, basket windows, clipping/high jumps and rare OAM ordering |
| Audio timing/mixing | $80:9DF3; $82:FD65-FF84; SPC/DSP driver | Command-dispatch extraction and SPC engine | Full CPU/APU timing denominator unknown | C isolates audio RNG; native dispatcher shares/consumes event words during interrupts | ROM BRR/SPC assets and host mixer; full timing equivalence absent | 60 command/RNG/source/pitch/volume cases; asset/DSP traces | PCM/event regressions, not cycle-equivalent gameplay | Synthesized ROM-derived audio; no capture/WAV shortcut | High commands/assets; low shared timing | Shared RNG, interrupt consumption, voice priority and long crowd/music continuity |

## Next implementation order

1. Match configured-start state and native scheduling; stop at the first real
   post-baseline divergence. Add checkpoints around actor commit, collision,
   ball, play request, clock and camera.
2. Complete human ownership/control, then connect the stripe helper through
   natural foul-to-free-throw input movies.
3. Port break/final raster children and every timeout/substitution menu with
   synchronized PPU/OAM/audio captures.
4. Map non-exhibition persistence and shared CPU/APU event ordering.

## Evidence standard going forward

A parity claim needs all three layers: a precisely bounded native capture,
zero-mismatch replay of every represented output, and production integration
tested under the same initial context and inputs. Ghidra/recomp mapping or a
long C-only regression can support that proof, but neither substitutes for it.
Until the strict differential reaches a matching baseline and bounded
post-baseline checkpoints, the honest project description is a substantial,
evidence-backed partial port with a CPU-only playable path and isolated human
helpers.
