# Scheduler implementation plan — 2026-08-31

Planning only. Baseline inspected: owner `f318478ef12586928c2e03b034a49ddf8bc508bd`.
No source, freeze, capture, build or expected-output changes were made for this plan.
The owner's new untracked `docs/completion-plan-20260831.md` is concurrent planning,
not an executable change. This ignored document refines its milestones 1B, 3 and 4.

## What is actually missing

The repeated Rules failure is a carried execution-phase failure, not an unknown
backdrop decoder or a reason to add a visit delay. The current root retest log
`completion-owner/build/hud-runtime-integration-v1/rules-reentry-preservation.log`
reports **158 differences, first native1176/C893 brightness**. Root identifies
that run as f318478's hud_cli.exe with current pack
`f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`;
the short exception log itself does not embed the invocation or those hashes.
Historical `completion-owner/build/rules-reentry-resumed-v1/report.json` retains
the full older-pack command and158 differences: its first brightness15 versus1,
forced blank1 versus0 and three scroll differences show C in the previous
publication stage. Keep those report identities separate, despite the same first
failure. Historical C873/C877 offsets do not replace this C893 mapping.

The source-derived work to reach the phase-sensitive wait is already available:

| Reusable component | Accepted boundary and evidence | Still outside that acceptance |
|---|---|---|
| Queue/wait v3 | 599 dispatches,126 publications,12 budget stops,4 M0 and16 M1 waits | Production driver, elapsed DMA, interrupt tail |
| FB46 v2 and FB30 | 112814/36418 instruction states;16/4 payloads | Other streams/domains and wall-clock prediction |
| Backdrop producer v2 | 155750 states,40003 writes,27726 DMA bytes; four550560-CPU caller-to-return intervals | Shared hardware time and nonempty queue paths |
| Header v2 | 126 states,66 writes,8206 DMA bytes; four440-CPU intervals | The wait at EF1A and real interrupt scheduling |
| CPU sound prefix/init v2; SPC resident v3 | Ordered source work up to explicit unresolved I/O/sequence boundaries | Normal upload/startup, acknowledgements, timer/DSP/sequence continuation |
| SPC initializer v4 and F1 control v5 | RAM-clear source; exact control-write effects | Composition through the pending cycle, normal clock ownership, DSP/timer execution |

These are accepted standalone components, not production timing. In particular,
the old **26487-CPU routing residual is closed**: producer total550552 plus
caller JSL8 gives550560. Do not spend a new ticket deriving it again or turn its
3493454 intrinsic master clocks into a delay. Header intrinsic work is2764.
DMA, refresh and interrupts can separate those source operations.

The decisive remaining audio evidence also rules out a tiny timing constant:
all four backdrop paths have the same550560 producer CPU work, but the Rules
first/repeat interrupt tail differs22244 master clocks. Audio accounts for
22730 more and controller polling486 fewer. Only1428 of the audio difference
comes from34 additional failed polls; actual sequencer decisions must advance.
No phase-erasing synchronization point has been established. The old handler
hooks omit hardware entry/vector-JML/RTI:19 CPU/142 intrinsic master per NMI.

## Ownership and dependency order

Scheduler implements the tickets below; root alone integrates shared scene,
manifest and gameplay adapters; controllers owns gameplay/contact/human fixes;
auditor independently reviews source and verifier integrity. Begin implementation
in the fresh integration-based worktree root assigns, importing only identified
accepted components. Preserve this archival worktree and every old freeze.

Critical path: **S1 normal bootstrap → S2/S3 → S4 → S5 → S6**, then strict HUD/boot timing closure.
S2 and S3 may be split into source/hardware subtickets; their integration must
meet before claiming normal sound state. H1/H2 and B1 semantic work are useful
while advice or audit blocks this path. R1 starts early but its runtime cutover
depends on a real sound-command return/allocator. This is incremental ownership
repair, not a production 65816/SPC opcode interpreter or whole-engine rewrite.

### S1 — First implementation-sized ticket: normal reset/upload/resident handoff

The existing Max consultation corrected the initial proposal to bridge only
the two standalone entries: that would leave the production initialization gap.
Instead implement composed cold reset → IPL/80:AB06 upload → resident entry over
one persistent CPU/SPC clock/bus owner. Root coordinates its lifetime alongside
`NbaGame.session`, **outside `NbaGame.scene`**: nba_game.c305 clears that entire
scene union on transitions. Current nba_game.c425 initialization starts the
license scene without the original reset/upload. A per-scene scheduler loses the state
the Rules fix needs to retain.

Begin with ROM plus a declared hardware power-on contract. Execute reset caller
80:8038..803F and the actual uploader/IPL transactions described under S3; the
1264-byte resident must reach ARAM through those writes, not a preloaded image.
At0384, keep live registers, both directional port arrays, staged inputs, ARAM,
timer stages and IPL visibility in that canonical state. After the real pending
bus-cycle service, use accepted `nba_setup_spc_control` to commit value30 once,
resume accepted `nba_setup_spc_init` at0387 with the resulting registers, and
continue to the pending **03DB OR A,$F3** read. The source-work document contains
an older MOV description; the reviewed opcode and integration correction say OR.

Deliver the normal-bootstrap composition, root-facing persistent owner/step API
and focused executable probe, not another report-only result. F1 post-cycle
commit charges no clock itself; charge each real bus cycle once in the owner.
Carry timer history and staged-port visibility through upload. Any unresolved
DSP/timer dependency remains an explicit stop; no guessed value completes it.
The bounded exit is a real source-produced0384 completion,0387 continuation and
pending03DB read, not a claim of complete initialized sound or Rules phase.

Gate: source instruction/bus order through both sides, exact preserved opposite
output latches, enable/reset edges, IPL overlay and08FF omission; repeat pending
step refuses mutation; nonzero poisoned component cases; fresh native prefix
and F1 differentials plus composed source checks. **The bootstrap gate itself
starts from ROM/power-on only, with no ARAM/register/port snapshot seed.** Native
component prestates may supplement it, never replace it. Old probes stay exact.

### S2 — Own the SPC hardware clock, then finish resident initialization

Translate timer divider/output/target behavior, read-clear registers and enable
edges from pinned local hardware sources; carry pending CPU inputs separately
from visible inputs and from SPC output latches. Specify actual CPU/SPC clock
conversion, sampling and publication order, including reset phase. F1 cannot be
treated as an independent time advance. Every effect occurs at its real bus cycle.

Implement the original03DD..043F continuation, relevant DSP6C read/write effects,
timer target FA=10 and F1=01, directory/pointer setup and F5..F7 writes into0447.
Then implement poll/timer service and the command bodies needed by S3/S4. Resident
command05 acknowledges at0613 before its DSP/body work; preserve that order, but
do not pretend command05 alone handles CPU command0B. Keep uncovered command and
DSP behavior as explicit stops until translated.

Gate: timer target0/wrap/output-clear and all enable edges; directional latches
and input visibility at neighboring clocks; instruction-complete boundaries;
source-only initial states plus native I/O chronology and full owned state.
Advance elapsed DSP work from its own reset state where it affects a read or
acknowledgement. A guessed DSP read value, fixed polling latency or prerecorded
response cannot satisfy this gate. Full audible mixing is a later gate, but an
omitted DSP dependency may not be presumed irrelevant to acknowledgement timing.

### S3 — Finish CPU sound initialization after S1's normal IPL/upload

The bootstrap owned by S1 includes reset caller80:8038..803F →80:AB06 and its IPL
consumer: BBAA idle signature, CC startup, incrementing byte tokens, destination
and terminal entry descriptor. ROM00:C683..CB7A contains the1272-byte stream;
payload C687..CB76 maps to ARAM0380..086F (1264 bytes). This is literal ROM
provenance, not permission to seed an initialized snapshot or bypass upload work.

Do not reimplement S1's uploader here. Connect its resident S1/S2 state to CPU9B73,
finish the channel-off/acknowledgement
continuations, and translate normal callers82:AD48/ABF2, sequence/resource startup
9CC8/9829 as reached. Carry the actual table bank; reset initializes5A/5B to zero,
later Setup initializes82. Preserve nested byte guard53 underflow, not a bool lock.

Gate: source-produced uploaded bytes, all port directions/tokens and transitions,
normal reset through the first completed sound initialization without a captured
prestate/return-A. Reject premature completion and unimplemented command bodies.
Extend to title/Setup sample uploads actually needed to establish that route.

### S4 — Complete the CPU sound sequence and interrupt tail needed by Rules

Extend80:A137 after the accepted pending read. AAE6 waits for *idle zero*, then
writes channel/data and command0B; AAFC separately waits for the echo, then AB01
clears the port. Implement the two longer A2CE sequence paths (first readA9E5),
tempo0643/0645, voice/sequence tables and command work from normal initialization.
Carry all of it across page transitions and menu dwell. No stream-upload
A047/A06A/A08F ran in the existing46 NMI sample; do not make that unused family
the first blocker, but cover it later when the route inventory reaches it.

Implement actual NMI caller/return work, CB8F controller polling, A137 call and
RTI with source-width/stack/guard ownership. Preserve05CB reentry guard,059C epoch
suppression,05C2 callback and0564 increment order. The increment does not finish
the interrupt: waiting code resumes only after RTI.

Gate: all46 native tail instruction/effect projections as differentials; then
predict those same tails from prior C-produced state, including first/repeat
sequencer work and menu dwell. No measured NMI duration is an input. Joystick
poll work follows actual latch/auto-read timing, not a fixed tail allowance.

### S5 — Shared CPU/NMI/DMA timeline and resource driver

Compose existing ordered producer/header work, queue and waits under a persistent
hardware owner: CPU intrinsic bus costs, memory speed,40-clock refresh, PPU phase,
pending NMI and instruction completion, DMA alignment/channel/mode/remaining work.
Budget/resource ownership remains source-derived; work may pause at an actual
bus/instruction boundary and resume after interrupt/DMA completion.

Implement/prove mode1 low/high-port phase across split segments before using it
for all queued graphics. Retained job412 at native1511 is a2979+3357-byte split;
remaining-counter callback order and initial low/high phase cannot be inferred
from which bytes changed. Immediate fixed mode8 and observed contiguous mode1
proofs do not establish this split. Preserve no-op writes to already-zero bytes.

Keep incoming M for86B0: EF1A is word wait,8959 byte wait. Verify widths from ROM
and predecessor flags; old80:8332 wide-CMP decode was wrong (M1 CMP#D6,8334BEQ,
8336LDA). Do not add a reset at each scene or a visit-specific frame schedule.

Gate: source bus chronology/clock identities and write strobes, queue wraps and
budget stops; source/hardware-controlled neighboring NMI/refresh/DMA boundaries;
full buffers including25 unchanged-value return writes. Integration must replace
the old schedule for the covered route, not run a second shadow publication owner.

### S6 — Cut over and close the real Rules journey

Root wires the new driver into normal Setup entry/update/return while scheduler
owns the narrow resource/transition changes. Use retained first divergence to
locate the first new mismatch, rather than moving capture offsets. Predict header
loaded epochs **72/15/71/15** and wait return **73/16/72/16** from carried state.

Gate: zero of the158 retained RGB/PPU/VRAM mismatches, complete first/second open
and return buffers, changed Custom values and immutable prior UI/resource gates.
Then run at least a second independent normal journey with withheld repeat
counts, legal input dwell times, Rules/Options ordering and values to reject a
fitted two-visit schedule. Normal route has no CPU/SPC
snapshot, injected phase, after-state, captured port reply or empirical delay.
Until this succeeds, whole Rules reentry/phase remains FAIL regardless of leaves.

## Shared RNG and presentation tickets

### R1 — Replace the *actual* legacy audio RNG/event owner coherently

Current nba_game.c728 calls `nba_audio_dispatch_gameplay_events` after Tipoff.
nba_audio.c702 uses private `gameplay_audio_rng_state`, initialized9146 at730,
and the old edge latch. `nba_audio_events` is absent from the40-source manifest;
its handoff explicitly says independent review pending. Do not describe its2612
native projections as accepted runtime behavior. HUD CE36 now does consume the
shared Tipoff07F6, so audio remains a real separate ownership gap.

First audit the event candidate and implement source80:9DF3 command allocation/
return-A plus9F0F queue effects. Bounce uses that return; captured return-A in the
leaf probe is not a runtime implementation. Cut over events, clears and RNG as
one integration: all callers use the canonical word at original dispatch order.
Coordinate with controllers' gameplay RNG/41876 work before changing trajectories.
Use a call-site/ordinal ledger to find first divergence, not a saved C score.

Gate: repeated same event on successive dispatches, all bit families/unknown bits,
Crowd OFF behavior, command returns and owned queue writes, real caller order;
muting only the host playback sink leaves gameplay/event state unchanged. Source
Crowd OFF can change RNG use and is not equivalent to muting that sink. Fresh long
CPU/regression journeys follow each consumer restoration; failures are attributed.

### H1 — Finish HUD children by original selector, starting with statistics

Start with observed83:D333 kind6, then the actual83:DA12 kind17 and83:EC60 foul
clear branches. Translate child text/resource allocation and state writes on the
current indexed canvas; no replacement score panel for every basket. CE36's
reject-loop RNG order stays intact. Source event4939 and assist493D producers
remain gameplay-owned interfaces, not counters invented by the HUD.

Gate per child: fresh before-only native projection, source-only selector/bounds
cases, map/canvas/CHR/queued upload bytes and exact stale-panel clearing. Normal
first-score/assist/foul routes must stop producing those pending_routine logs.
Expand the entire source44-entry selection table and advertisement resource
upload owners into explicit ledger tickets; three observed kinds alone cannot
close the HUD. Scheduler implements, root maps gameplay fields, auditor reviews.

### H2 — Resolve paused08DE ownership and the strict clock crossing

Inspect85:ED0D/EDAC registration/callback order and pause callers before touching
the C early return. The native timer callback precedes the live-game clock gate;
pause holding actors/0928 does not prove it holds08DE. Translate the distinct
timer owner, including positive decrement and CC10 zero-to-FFFF expiry.

Gate: paused/unpaused positive/zero/negative timer, callback enabled/disabled,
menu return, foul and score panel clearing; naturally entered pause validates
the visible result. With S4/S5, preserve the exact innerBBE9 read ordering so the
two strict08F6 native observations no longer need atomic-parent allowances.
Current bounded2747926/2747930 HUD comparison is not full atomic-parent acceptance.

## Boot, graphics provenance and audio completion

### B1 — Boot control before full timing; then shared-driver handoffs

Translate license/legal wait and skip-input continuations, EA hold/fade and title
handoff using original caller state. Legal first181 waits versus second121 waits
and exact Start1000 acceptance in the second loop are distinct. Native forced
blank construction/audio work is work, not another guessed pause.

Reuse accepted303 EA motion frames and original text/font resources; do not
re-repair their rasters. Their C345+motion mapping is only phase selection.
Connect S3–S5 to replace host lead-in/frame timing and resource publication.
Gate: cold boot with no input and presses before/during/after each legal skip
window, consecutive native phase/PPU/frame comparisons through title and Setup,
without retiming offsets chosen from captures. Keep current renderer tests too.

### B2 — Remove remaining capture-derived resource dependencies by format/domain

Inventory remaining indexed native-memory extraction independently of RGB
provenance. Intro75 includes11776 bytes of an11904 declared decompression output;
the prefix alone is not whole-stream proof. Its static format30 resources and
font-palette AF:F2DC still require their own source decoder validation. Accepted
FB30 AEA0AF is a bounded codec proof, not automatic acceptance of every format30
stream. Extend by real stream bounds/branches and rebuild exact affected IDs;
unchanged common assets must remain byte-identical. Do not replace entire packs
or ship trace/ARAM snapshots as a shortcut to source-produced state.

### A1 — Options/gain and full audible behavior

Implement native87:8C2D..8C65 gain min(3*value,127), routing15C5/15C7→80:9C47
and0627/0628, then off/mono/stereo and crowd consumers. Current host setup gain
functions are not that translation; gameplay uses direct fixed voice gains.
Keep source command/sequence progression distinct from host sink availability.

With S2–S4/R1, replace snapshot/DSP-event and hardcoded intro audio7 schedules
with source initialization, commands, voice priority, envelopes and sample
consumption. BRR source provenance and correct command counts are necessary but
do not prove audible sequencing/mixing. The current nba_spc output-port discard
is not a valid acknowledgement owner.

Gate: every0..45 volume, discrete modes and crowd choice through menus and real
Setup/gameplay callers; exact command/state/DSP traces; deterministic PCM on a
declared sample/time base plus listening review for intro, transitions, crowd,
overlapping effects, pause and return. Retain original channel/sequence quirks.
No WAV capture is a production sequence and no silent no-op passes audio closure.

## Concrete constraints and review routine

- Only one coordinated native Mesen slot; controllers/HUD capture priority as
  assigned by root. No emulator/build/capture was run for this plan. Use private
  portable executable/settings/save homes, per-child environment and hidden
  processes; never stop another game's emulator. Retain failed/time-out runs.
- Normal journeys start from actual reset and ordinary CPU/controller input,
  with no state seeds. Controlled original-ROM states are valuable separately
  labeled component tests, never normal-phase evidence or expected-after inputs.
- Pin actual ROM, source closure, tool scripts, settings, executable and pack.
  Inspect M/X/D and DB/DP at every boundary; require raw chronology, exact numeric
  domains, duplicate-key rejection, complete stop markers, exact fetch bytes,
  process int-zero status, unchanged source identities and native scope bounds.
- Private fresh /W4 /WX builds and actual caller coverage precede freeze. Auditor
  rebuilds independently and reruns unchanged corruption cases. A source pass
  with a verifier rejection does not become accepted by changing native data.
- Root integrates only an accepted ticket and reruns affected combined gates.
  Runtime wiring is a separate checkbox from a helper acceptance. Keep original
  bugs commented; do not classify current Rules timing, stale period state,
  duplicate actor aliases or a failed C assertion as an original defect.

## Review of root milestones1B/4 and existing Max question

Milestones1B/4 are directionally correct. Add corrected S1's normal bootstrap
and persistent non-scene owner as the next concrete code ticket,
make the already-completed producer residual explicit, and distinguish accepted
sound prefixes from an unimplemented whole interrupt/timer/DSP owner. Put R1's
legacy private RNG replacement before accepting stable full-game trajectories.
Start H1 and semantic B1 when unblocked; their final timing acceptance still
depends on the shared driver. Resource75/76 provenance is bounded, not finished
just because no PNG/WAV is read. Gain/voice/audio options need real consumers.

Existing read-only Max task: `01a05634-5316-78c0-bb36-f9cdfd3b562e`; parent has
already reactivated it. Its returned recommendation is incorporated in S1:
normal reset/upload, not standalone prestate composition, and a persistent owner
outside the cleared scene union. Further dependency order is resident03DB /
timer048B / commands → channel-off/sample/sequence/A2CE/A137 → CPU bus/refresh/
DMA/controller/NMI through RTI → strict HUD read boundaries. No new task or
duplicate dispatch is requested here.

One precise question to carry into that consultation: **From reset IPL/upload
through resident0380 and CPU9B73/9CC8, what is the smallest source/hardware state
closure that determines the first AAE6 idle read and AAFC command0B echo during
A137, including DSP/timer and staged-port visibility dependencies, and what
instruction/bus observation would falsify that closure?** Request source-backed
advice, not code, guessed delays or captured replies. The 22244-master tail
difference and absence of a proven phase-erasing boundary are the minimized
blocker; codecs and routing costs no longer are.

## Evidence read for this plan

Paths below are relative to `completion-owner` unless marked otherwise:

- `docs/scheduler-integration-checkpoint.md`, `codec-integration-checkpoint.md`,
  `producer-header-integration-checkpoint.md`, `setup-producer-source-work.md`,
  `setup-header-source-work.md`, `setup-scheduler-consultation-plan.md`.
- `docs/sound-composite-integration.md`, `spc-initializer-integration.md`,
  `spc-control-integration.md`, `setup-sound-prefix-source-work.md`,
  `setup-sound-initialization-source-work.md`, `setup-spc-init-source-work.md`,
  `setup-spc-resident-source-work.md`; accepted headers/source at those boundaries.
- `docs/rules-reentry-resource-audit.md`, `rules-publications-independent-audit.md`,
  `build/rules-reentry-resumed-v1/report.json` and `run.log`, and current
  `build/hud-runtime-integration-v1/rules-reentry-preservation.log`.
- `docs/gameplay-hud-integration.md`, `gameplay-hud-lifecycle-repair.md`,
  `audio-event-consumption.md`, `gameplay-audio.md`, `options-test-ownership-audit.md`;
  current nba_game.c/nba_tipoff.c/nba_audio.c/nba_gameplay_hud.c and40-source manifest.
- `docs/intro-indexed-resources.md`, `intro-text-independent-audit.md`,
  `intro-exact-audit.md`; `build/hud-runtime-integration-v1/cpu-regression.log`.

Historical documents can describe old production state. Current source/callers
and exact report commands take precedence; apparent old acceptance claims are
not silently broadened to the current executable.
