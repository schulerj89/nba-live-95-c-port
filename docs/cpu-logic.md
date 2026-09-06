# CPU logic workflow

The native gameplay scheduler separates actor physics from behavior decisions.
`$87:8EFB-$8F92` visits all ten actors with physical delta `$C6=2`.
`$87:8F13-$8F5E` eases display direction before `$87:AAB2` advances
animation; `$85:963D-$985F` then resolves locomotion and commits the velocity
selected on the previous actor pass. Global ball, contact, and role work
follows. The later `$87:9244 -> $87:9BD3/$9BD0` mode jump table dispatches the
actor's current `+$5E` behavior. Production preserves this complete phase
order for modes seven through ten: `cpu_update_all_actors` eases and advances
the existing animation before committing old velocity, while the later
behavior sweep runs `$86:994C`, `$86:A5B0`, `$86:C6AD`, or `$86:F0B7` after
globals. Direct native-vector adapters intentionally call only the bounded
behavior being replayed.

The dispatch table currently maps modes 0 through 17 to no-op, ordinary CPU
offense/defense continuations, dead-ball and action holds, knockdown, target
override, receiver, shooter, layup, and pass executors. Each exact routine is
ported and credited separately. A table target in production does not by
itself mean the full target routine has native parity evidence.

## Mode seven dead-ball hold

`$86:994C-$99C3` consumes these inputs:

- full-word live state `$0936` and offense group `$093A`;
- current actor pointer `$96`, actor slot `$C2`, and physical delta `$C6=2`;
- integer position `+$04/+$08/+$0C`, planar velocity `+$0E/+$10`, signed
  controller assignment `+$16`, timer `+$60`, direction latch `+$66`, movement
  boost `+$72`, and current/requested direction `+$4E/+$50`;
- actor team/roster identity and global owner `$093E` through the existing
  movement-profile and velocity child.

The routine takes the following paths:

1. Any full-word live state other than `$0082` requests animation state 3 and
   calls `$86:9846-$986C` to restore mode 1 or 2, behavior timer `$2F`, and
   cleared timer/flags/status.
2. In state `$0082`, a nonzero integer Z word returns immediately. Fractional
   Z does not hold the timer.
3. A grounded actor subtracts two from `+$60`. A negative 16-bit result takes
   the same state-3 and restore path.
4. A CPU actor calls `$85:B3AA -> $85:A82C` with world target `(0,0)`. This can
   accelerate toward origin or damp existing velocity to zero. Human actors
   preserve velocity and boost.
5. Zero post-child planar velocity requests state `$10`, copies `+$50` to
   `+$4E`, and clears `+$66`. Moving actors request state `$0E`; any nonzero
   prior `+$66` flips `+$4E` by four and copies it to `+$50`, then the routine
   writes `+$66=1`.

Animation requests use the ROM-backed channel command equivalent of
`$87:B3BD`. Resource resolution remains in the later animation/render stage,
so this routine preserves `+$2A/+$2C` resources and does not publish the body
mirror bit early. The command tables and per-player movement profile come from
the user-supplied asset pack (`build/nba95_assets.pak` by default); no asset
bytes are stored in the witness fixture.
Parity is limited to a valid pack with the selected team/roster movement
profile and the canonical actor-pass delta `$C6=2`; the host fallback profile
used after a failed lookup is not a native-parity claim.

`tests/fixtures/cpu-mode-seven-witnesses.json` contains 19 compact calls from
two byte-identical genuine-entry Mesen captures. Each call enters at `$86:994C`
and exits at `$86:9961` or `$86:99C3`; no PC, stack, ROM, RNG, or child result
was patched. `tools/verify_cpu_mode_seven_vectors.py` and
`tools/cpu_mode_seven_vector_probe.c` compare all 50 represented fields with
an exact output shape. They cover full-word state comparison, positive and
negative integer Z, grounded fractional Z, timer `2/1/$8002/$8001`, X-only and
Y-only movement, zero versus arbitrary nonzero direction latch, human boost
preservation, CPU damping and steering, post-damping stationary selection,
animation boost remapping, matching state, and a negative animation lock. A
separate full-scheduler check proves the old velocity moves the actor, a newly
installed stationary `$10` animation is not cadence-advanced in the same pass,
display-direction easing selects the pose resource before physics, and
live-state/ownership changes injected at the real post-physics observer
boundary are visible to the later mode-seven dispatch.

The changed whole-game call order exposed `$FFFF` at retained C trace frames
58472-58473. This is valid `$80:CEE7` state, not missing telemetry: the trace
holds `$789E`, advances to `$F13C`, holds, advances to `$FFFF`, holds for two
frames, then reaches `$D975` through `$E279` in two calls. The focused probe
checks `$F13C->$FFFF->$E279` and zero recovery to `$9146`; this adds no RNG
routine credit.

The host narrows controller assignment and facing to their established runtime
types. The retained native calls cover the real `$0000` human and `$FFFF` CPU
controller words and directions 0 through 7; arbitrary noncanonical high-byte
controller or direction words remain outside the claim. The mode-seven phase
correction is deliberately scoped; other behavior modes retain the existing
partial whole-game scheduler model. Whole-game launch state, scheduler
history, and RNG history also remain outside this bounded routine result.

## Mode eight knockdown recovery

`$86:C6AD-$C758` consumes the selected actor, canonical physical delta
`$C6=2`, integer Z `+$0C`, velocity `+$0E/+$10/+$12`, status `+$28`, signed
landing selector `+$56`, contact timer `+$60`, landing marker `+$66`, actor
team group `+$6E`, and flags `+$7E`. It also reads offense group `$093A`,
owner `$093E`, and ORs the shared event word `$13E7` on the bounce branch.

Every call ORs flags with 6. A nonnegative selector, zero vertical velocity,
and zero integer Z trigger landing even when fractional Z is nonzero. A
nonnegative marker sets event bit `$0100`, installs vertical velocity `$00F0`,
and arithmetically halves planar velocity; a negative marker clears planar
velocity while the landing gate has already required vertical velocity zero.
Both paths write marker `$FFFF`. The routine then subtracts
two from `+$60`. Its phase relative to `$36` clears status bits `$18`, setting
`$10` for phases 10-19 and 30-39 or `$08` for 20-29. A negative timer calls
`$86:9846`: actor group `+$6E` versus `$093A` selects mode 1 or 2, behavior
timer becomes `$2F`, and timer, flags, and status clear. Contact inhibit
`+$5A` clears before the current-owner override changes the mode to 11. The host has two
projections of native actor `+$60`; `contact_action_timer_raw_60` and
`reaction_threshold` are synchronized on contact, every mode-eight decrement,
and expiry.

The scheduler advances the existing animation and commits prior velocity
before contacts. Its bounded `$87:90A5-$90C2` integration skips early
countdown for actors already in mode eight, then applies signed wrapping
countdowns to mode-eight `+$7A` and `+$5A` after contacts and before dispatch.
Thus a contact-installed value 30 becomes 28 once in that pass, while an
initial mode-eight record is not counted twice. Contact writers
`$86:C223-$C22F` and `$86:CB6A-$CB76` cancel upper and lower channels through
`$87:B538/$B555`, then install action 35 or 36 on both through `$87:B3BD`.
The new action begins its cadence on the next actor pass.

`tests/fixtures/cpu-mode-eight-witnesses.json` contains 19 compact calls from
two byte-identical genuine-entry Mesen captures. Every call enters at
`$86:C6AD`, exits at the sole `$86:C758` return, and collectively covers all
67 instruction starts. The 27-word projection includes overlapping DP
`$0046/$0047`, X/Y/Z integer and fractional words, both host timer
projections, unchanged `+$56/+$7A/+$A8`, and the complete affected actor
state. It covers selector and vertical early gates, fractional landing,
signed halves, bounce and settle, timer `2/1/0/$8002/$8001`, all presentation
phase boundaries, owner mode 11, and actor-group mode 1/2 restore. Exact child
counts pin the three `$86:9846` calls. The prior production leaf mismatched all
19 final-shape replays; the corrected direct replay has zero mismatches.

The production probe separately checks an initial mode-eight pass, signed
`+$5A/+$7A` countdown, fractional landing after common physics, and the two
real high-speed contact actions against the verified animation command model.
It follows action 35 through its next pass to prove both channels and resources
remain on the knockdown. A synthetic observer-injected mode-nine-to-eight
record isolates the post-physics phase and proves same-pass cooldown and
dispatch without a second coordinate commit; that case is not an actual
contact-writer replay. The natural C trace first diverges at frame 2686 when a
real `$86:C91E` pose contact installs mode eight on actor 6.

The leaf itself has no asset dependency. Contact animation and later cadence
require the ROM-derived animation tables in `build/nba95_assets.pak`. Parity is
limited to canonical `$C6=2`, the captured word domain, and the bounded late
mode-eight/mode-nine cooldown slice. Broader scheduler state and contact/RNG
history remain outside the routine claim.

## Mode nine timed target override

`$86:F0B7-$F0FC` consumes the scheduler-selected actor pointer and physical
delta `$C6=2`, plus actor integer position `+$04/+$08/+$0C`, velocity
`+$0E/+$10`, saved target `+$56/+$58`, mode `+$5E`, timer `+$60`, saved mode
`+$62`, boost `+$72`, recovery inhibit `+$7A`, velocity direction `+$A2`, and
animation channels including alternate-lower selector `+$A8`. `$85:A82C`
also reads exact live state `$0936`, owner `$093E`, and the selected player's
movement profile from the user-supplied asset pack.

After `$87:AAB2` advances the existing animation and `$85:963D` commits the
old velocity, global ball/contact/role work runs. `$87:90A5-$90B2` then
subtracts two from nonzero `+$7A`, storing the wrapped result only when it is
nonnegative and otherwise zero. The later `$87:9244` dispatch follows these
branches:

1. Any still-nonzero `+$7A` restores immediately without decrementing `+$60`.
2. Otherwise `+$60` subtracts two. A negative signed result restores. Thus
   inputs 0, 1, and `$8002` restore, while `$8001` wraps to `$7FFF` and runs.
3. Remaining values at least 10 load target `+$56/+$58` and call
   `$85:B3AA -> $85:A82C` to select and install next-pass velocity.
4. Remaining values 0 through 9 call `$85:A82C` directly with the velocity
   direction in `+$A2`. The preceding physics pass refreshes this direction
   from old motion, so this is distinct from steering toward the saved target.
5. Restore clears `+$60` and `+$72`, copies saved mode `+$62` to `+$5E`, and
   sends a both-channel state-3 command through `$87:B3BD`. It does not call
   `$86:9846`, mutate a separate host action state, or publish new render
   resources during the behavior pass.

`tests/fixtures/cpu-mode-nine-witnesses.json` retains 16 compact calls from
two byte-identical genuine-entry Mesen captures. Every call enters at
`$86:F0B7` from the native dispatcher and exits at `$86:F0DC`, `$F0F2`, or
`$F0FC`, with no PC, stack, ROM, RNG, or child result patched. The fixture
compares 52 exact words, including overlapping DP `$0046/$0047`, and covers
all 28 owned instruction starts, both nonzero-inhibit forms, signed timer
boundaries, X/Y/coincident target steering, final-window directions and boost,
live-state/integer-height movement blocking, saved modes 4 and 6, `+$A8`
values 0 and 1, and locked animation channels.
`tools/verify_cpu_mode_nine_vectors.py` enforces fixture identity, field and
call counts, exact PC/path union, per-case child counts, and exact outputs.
Its production `nba_tipoff_update` checks additionally prove old-coordinate
commit, old-animation advancement, refreshed stale `+$A2`, different target
and final-window directions, state-3 queue timing, and post-physics recovery
transitions `1->0`, `3->1`, `$8002->0`, and `$8001->$7FFF` before dispatch.
The retained C regression first reaches mode nine at frame 538. At frame 540,
the old and corrected builds have the same committed actor position and timer
28, while the corrected `$86:F0B7` changes the next velocity from `(19,46)` to
`(-69,-44)` before the first frame-600 raster anchor.

Parity is limited to the canonical `$C6=2` actor pass, valid pack-backed
movement/animation tables, the captured target and direction word domain, and
saved modes 4/6 produced by the verified anticipation callers. The host
narrows directions to its established byte fields. A mode already equal to 9
is decremented at the verified late point. The bounded mode-eight integration
also handles a mode-9 actor changed to mode 8 before that point, including the
native `+$5A/+$7A` countdown and same-pass mode-eight dispatch. Whole-game launch
state, scheduler history, and RNG history also remain outside this bounded
routine result.

## Mode ten receiver

`$86:A5B0-$A628` consumes the scheduler-selected actor pointer and physical
delta `$C6=2`, actor timer `+$60`, group `+$6E`, mode `+$5E`, behavior timer
`+$64`, flags `+$7E`, and status `+$28`. Its globals are full-word live state
`$0936`, offense group `$093A`, owner `$093E`, pass actor `$0942`, auxiliary
selector `$0944`, receiver `$0946`, ball activity `$0948`, attempt latch
`$094A`, inbound transfer `$09B8`, pass-active `$09C4`, and rules word `$17D5`.

With a nonnegative receiver, `$A5BA-$A5D8` first evaluates the N/Z flags from
the wrapped subtraction `($0944-5)`. A nonnegative nonzero result masks the
selector to three bits and follows the native `$87:9C71` pointer table.
Indices 0 through 4 read the held-input word from controller records; indices
5 through 7 read the signed integer Y word from actors 0 through 2. A
nonnegative referenced word normalizes `$0944` to the masked index; a negative
word preserves it. This includes full-word wrap cases such as `$8004`, `$8005`,
and `$FFFF`; it is not an ordinary greater-than-five comparison.

The routine then subtracts two from actor `+$60`. A nonnegative signed result
returns without other writes. A negative result reads `$17D5`; both native
paths call `$86:9846`, which selects mode 1 or 2 from actor group `+$6E` versus
offense group `$093A`, writes behavior timer `$2F`, and clears timer, flags,
and status. Both paths then clear live state. An initially negative receiver
calls the same restore directly and preserves live state only when its exact
full word is `$0082`. Every terminating path sets `$0942/$0944/$0946` to
`$FFFF` and clears `$0948/$094A/$09B8`; owner `$093E`, pass-active `$09C4`,
the ball record, coordinates, velocities, animation channels, lookup inputs,
and represented scratch remain unchanged.

`tests/fixtures/cpu-mode-ten-witnesses.json` retains 18 calls from two
byte-identical genuine-entry Mesen captures. Every call enters at `$86:A5B0`,
exits at the shared `$86:A628` return, and collectively covers all 48 owned
instruction starts. The 68-word projection covers the full modeled write set
and preservation set, including controller-held and actor-Y lookup inputs,
coordinates, velocities, animation channels, and scratch. Exact child counts
pin the six `$86:9846` calls. The cases cover invalid receivers with both live
outcomes, controller and actor table signs, selector normalization and wrapped
comparison boundaries, timer `2/1/0/$8002/$8001`, both `$17D5` paths, and
actor-group mode 1/2 restore. The pristine `f7e95cd` implementation mismatches
10 of 18 final-shape calls; strict replay of the corrected production leaf has
zero mismatches.

The production probe proves old velocity and animation advance before the
later mode-ten behavior, global inputs published at the post-physics boundary
are consumed in that pass, common edge handling can clear `+$60` before the
leaf expires the receiver, and restore consumes the behavior pass. One further
even pass advances a valid ordinary locomotion pose through pack-backed
resources without synthetic exact-jump animation. Two passes installed through
`nba_tipoff_begin_rom_pass` exercise both preinstalled passer/receiver slot
orders through the production dispatcher and decrement the receiver timer
exactly once. Dynamic pass installation during the actor sweep is supported by
the reviewed phase order but is not exercised by those slot-order cases.

The dead-ball possession parent now reaches that same scheduler helper instead
of dispatching every actor behavior on every 60-Hz presentation frame. The
focused public-entry checks prove an ordinary odd frame leaves receiver timer,
physics, and animation unchanged; the next even pass commits motion and
animation before one timer subtraction; and an explicitly pending odd dispatch
clears its latch and runs behavior once without rerunning physics or the
mode-eight/mode-nine prepare pass. A real state-`$82` pass installed by
`nba_tipoff_begin_rom_pass` starts its receiver at `$0028`; its passer releases
after 25 outer updates with receiver timer `$000E`, before mode-ten expiry.

The retained C trace first differs at frame 160. Frame 159 has no pass and
actor 9 is in mode 2. On frame 160, matching global pass work installs actor 9
in mode 10 with timer 39; the corrected later behavior sweep alone subtracts
two and stores 37. This is production phase evidence for same-update
consumption after global pass creation, not a whole-game native trajectory
claim.

The scheduler-specific correction first differs from the initial mode-ten C
build at frame 947. Frame 946 enters live state `$82` on an even actor pass. On
the following odd frame, the initial build advances every normal actor decision
while the corrected parent retains every actor until the due frame 948. This
attributes the later corrected C-only image trajectory to the shared dead-ball
cadence without extending the bounded `$86:A5B0-$A628` parity claim.

The leaf itself has no asset dependency. The production continuation uses
ROM-derived animation tables from `build/nba95_assets.pak`. Parity is limited
to canonical `$C6=2`, represented full-word inputs, and the bounded late
mode-ten dispatch. Broader scheduler, pass-creation, and whole-game RNG history
remain outside this routine claim.

## Mode twelve shooter

`$86:B769-$B978` is the mode-twelve shooter parent selected by `$87:9244`.
The production actor pass first eases displayed direction `+$52`, advances the
existing animation, and commits `$85:963D-$985F` physics. Global possession and
contact work then runs before the late behavior scheduler calls mode twelve.
The parent therefore computes the next pass's velocity and facing; it does not
move the actor privately. Lost ownership restores mode 1 or 2 through
`$86:9846` and consumes that pass. A pending behavior acquired after the common
pass can run on the following odd presentation frame without repeating either
physics or animation.

The parent reads raw owner `$093E`, activity `$0948`, free-throw state `$0978`,
the cached controller record pointer `$090C`, controller held word `+$08`, team
context anchor `+$0A`, and actor position, velocity, controller, pose, movement,
direction, timer, and flag words. `$86:B791-$B7CA` marks the live shot, records
the actor's integer X in `$0922`, and calls `$87:B832`. The C adapter uses the
current raw `+$2A/+$2C` resources even after a newly queued animation has made
render resolution pending, changes only ball integer XYZ, preserves ball
fractions and host owner/state projections, and reproduces the overlapping
DP `$0047-$0048` result in `scratch_0047` and the high byte of
`scratch_0046`.

The branch matrix covers same- and other-group lost-owner restore; pump phase
3, phase 4 accumulator `$05FF/$0600`, and two-channel cancellation; activity
delay, signed wrap, `$001C` jump crossing, and free-throw jump suppression;
stationary and moving sidesteps at anchor 119/120 and absolute X 55/56 with
both RNG-bit outcomes; grounded human and CPU pump handling; lower accumulator
`$05FF/$0600` and both facing turns; human vertical thresholds `$FE80/$FE81`,
held/released input, and free throws; and CPU vertical thresholds
negative/zero/`$005F/$0060` with RNG and free-throw variants. The turn gate
reads and writes movement direction `+$4E`; displayed direction `+$52` remains
owned by the preceding easing pass. Six release witnesses enter the complete
`$86:9D6E` launch child once, publish authoritative roster shot statistics,
synchronize the actor mirror afterward, publish human controller counters,
and clear `+$60/+$7E` after launch.

`tests/fixtures/cpu-mode-twelve-witnesses.json` retains 37 calls from two
repeated genuine-entry Mesen captures. Their full raw files differ only in
unrelated widened-window bytes; all 123 bounded input/output words and all
paths agree. The calls cover all seven RTL exits and the exact union of 213
owned instruction starts, whose set hash is
`8687118903ea44ad124933f7bec71f23e203cef0133072afedb8b71a610502a6`.
Aggregate child counts are 32 pose attachments, 11 directions, 11 lower
cancellations, 10 animation installs, two restores, one upper cancellation,
and six launches. The raw vector hashes are
`0156643bfbd1c72cdd623a217dff36d84b63da33128f6d60a692ea7d8f6f2c7b`
and `99bafc61ad3a1c5d474bc6c406907cf0257f848aad6afafc30c86957f1d8b605`.
The structural sources are the Ghidra function export
`cpu_gameplay_bank86_functions.c` with hash
`c3ce476090ee9a9ec970ce9b9ccb2fdeea42936f8a3ff9184880b609fff3b199`
and the Ghidra Bank `$86` instruction listing with hash
`f642199de28e016061db579f28a28f60c772de70a6a2a6712ded609e03f7d63c`.
The pristine `44cb6c0` parent mismatches all 37 calls; the production replay has
zero mismatches.

The direct free-throw caller `$87:9F11-$9F75` now uses the same parent. Its
state-9 preparation clears actor `+$4A`, cancels both channels, queues command
16, and falls through to the parent in the same actor pass. Each call saves the
full 16-bit assistance option `$17BF`, forces one around mode twelve, restores
the saved word, and advances state 9 only when raw owner `$093E` is negative.
The state-10 handoff preserves the launched ball and does not run another host
ball-physics step. Four repeated native caller witnesses compare 102 gameplay
words, including RNG and match clock `$0928`, at `$87:A017`: prep and hold
remain in state 9, while attempts two and one release to state 10 with one and
zero attempts remaining. The release calls span two native frames while
`$85:95DB` executes graphics/DMA work; that timing and volatile NMI state are
outside the portable adapter.

`tools/verify_cpu_mode_twelve_vectors.py` pins fixture identity, exact ordered
fields and calls, source hashes, child counts, boundaries, the instruction-start
set, and strict 16-bit JSON words before replaying production. Its pack-backed
probe also proves the persistent roster record wins over a stale actor mirror.
Public `nba_tipoff_update` cases prove common movement and animation execute
once before the late parent, restore consumes its pass and ordinary behavior
resumes on the next due pass, and an acquired pending event runs once on an odd
frame without repeating the common prefix. The free-throw probe covers
same-pass prep/body, raw-pose attachment, full assistance restoration,
ownership-gated completion, and the no-extra-physics state-10 handoff.

The parent evidence uses native actor slot 9, player record `$44EB`, and
controller words `$FFFF` or zero. The free-throw caller evidence uses slot 0.
The byte fields are bounded to the canonical 16-bit encodings observed by
native code; the synthetic scheduler case with actor 0 does not add native
side-zero or controller 1-through-4 coverage. The free-throw captures omit
player record `$416B`, so their caller projection excludes its statistics and
stamina. The parent witnesses prove the child statistic mutation contract for
captured player `$44EB`; exact `$416B` values remain outside. DP `$06` and
native graphics/NMI state are unmodeled. Host `ball.owner_actor` and
`ball.state` are integration projections rather than invented native words.
The pack-backed animation and launch tables come from
`build/nba95_assets.pak`. These results establish the bounded parent and direct
free-throw caller behavior, not a matching whole-game trajectory or complete
human-control system.

## Mode thirteen carried-ball close finish

`$86:A7DA-$A993` is the mode-thirteen parent selected by
`$87:9BD3[13] -> $87:9C49`. The production scheduler advances animation and
commits `$85:963D` physics once before globals and the late parent dispatch;
the parent owns no private motion pass. A lost raw owner `$093E` clears
`$1866`, restores the actor through `$86:9846`, and cancels pass activity
through `$86:A613`. A retained call clears actor `+$4A`, publishes the actor
record in `$1866` except for selector six, subtracts canonical `$C6=2` with
16-bit wrap, and finishes on zero or a negative result.

The airborne disruption path uses signed wrapped differences from actor
`+$BA/+$BC`, with inclusive `+/-80` retained, and lower/upper animation work
before pose resolution and integer-only XYZ attachment. Ordinary positive
timers resolve and attach once, set ball VZ `$FE98`, and use raw
`timer-8` byte offsets into the two 30-byte facing tables. Values above 36
return before facing. Remaining values resolve again through raw actor `+$4E`,
publish displayed `+$52` except when raw facing is eight, attach through
`$87:B832`, and leave its overlapping DP `$0046/$0047` result visible in the
host mirrors. Ball fractions and `$0948` survive these attachment children.

The terminal child `$86:A9D0-$AA69` now publishes raw owner, live/dead-ball,
shot-value, ball-record, animation cleanup, signed special-six velocities, and
player/controller attempt statistics without clearing unrelated `$09F8` or
shot origins. Its adjacent `$86:986D` continuation uses authoritative
`$3FEF/$3FF3`, selects the hold or landing action, resolves pose point one,
and starts effect four. Mode fourteen's existing terminal caller retains its
native A9D0-then-986D chain; no other mode-fourteen behavior is claimed here.
The helper behavior is checked as a direct child of the parent, but its 54
instruction starts were not separately hooked and receive no new range credit.

Two repeated controlled Mesen captures retain 37 genuine A7DA entries, all
three exits, all 172 parent instruction starts, and 144 represented words per
entry/exit. The owned-start set hash is
`8099d3c9748e73c57a37a44dc0ae436c6bc9b42d4ffbb641b60d4214ff7cb282`.
The v4/v5 raw-vector hashes are
`72cd5c59ef0f042afb2f6cee5f4c6709a7cd5ad4caf68f7ab7ccd02e6c482fee`
and `b310ca85621089dffae7968b6cac9544ec6c457429b966c05f87061cc8b3bd46`;
their case and path hashes are
`96b5ec651e58a606b8a705613831dfa8c6dd8404fd3767a0882a61076ed8e989`
and `7e95fca6af7adad344750d17562ea85868796fe67b0564e33e5f287c116bd5e3`.
Aggregate children are eight restores, six lower cancellations, 12 upper
installs, 49 pose resolutions, 27 XY attachments, six Z attachments, six
launches, eight terminal helpers, eight landing continuations, 49 point
attachments, two pass cancellations, 14 stat calls, and three effects.
`tools/verify_cpu_mode_thirteen_vectors.py` pins all fields, cases, paths,
hashes, boundaries, and child counts before replaying production. All 37 x 144
comparisons pass, including stale host-owner poisoning. Eleven fixture
mutations spanning values, types, shape, provenance, paths, children, domain,
and ROM identity are rejected before the production subprocess runs.

The native witnesses use slot/id 5, actor `$39EB`, player record `$446B`, and
active lineup `$4779=2`; pack address `$AC:F636` maps this to host actor 5,
context 1, roster slot 2, and persistent index 14. Controllers are `$FFFF` or
zero. Period `$0926`, difficulty `$17AF`, and both context `+$08` basket
fractions are fixed zero. Actor `+$00` is an identity invariant only. The
projection retains B832 DP `$46/$47`, pose snapshots `+$34/+$36/+$3E/+$40`,
authoritative roster/controller stats, and integer/fraction words; other DP
and stack arithmetic scratch is excluded. The isolated `d221619` baseline
fails all 37 cases across 135 common mutable fields with 659 word mismatches;
seven new telemetry fields and two identity words are excluded from that
source-compatible comparison.

Inline `$86:A994-$A9CF` contains 60 table bytes and receives no instruction
credit. Resource 277 `NBSHOT1` now carries those bytes, the eight-byte B448
landing table, and mode fourteen's eight-byte B440 queue table in an exact
eight-range, 640-byte form. The loader continues to accept the exact prior
seven-range, 620-byte mode-thirteen form and five-range, 528-byte launch form.
Accessors reject layouts missing their required range and malformed inner
directories. Pack contract tests exercise all three through `nba_assets_load`
and `nba_shot_launch` using only local temporary mutations. Native ROM table
bytes remain absent from source and fixtures.

The production caller test proves animation and common physics run once,
intervening globals are visible to the late parent, restore consumes the pass,
ordinary work resumes on the next due pass, and a pending odd event does not
repeat common work. The synthetic raw-facing-eight descriptor test checks the
AEC3 source contract but adds no native capture coverage. `$094E` is seeded to
30 by this parent; its conditional consumer at `$87:8E5B-$8E7C` remains a
separate scheduler-option gap. Whole-game trajectory, mode-fourteen conversion,
controller values 1 through 4, and nonzero period/difficulty/context fractions
remain outside this bounded claim.

The long CPU capture first changes shared terminal globals at frame 808,
where `$86:A9D0` explicitly writes `$0966=$FFFF` and `$0930=$04B0`.
Actor and ball telemetry match the prior capture through frame 7000; their
first divergence is frame 12658, after the first mode-thirteen entry at 12656.
The activity regression permits attached `$0948=1` specifically for a valid
mode-thirteen owner: `$86:A7E4` seeds it and the attachment children preserve
it. The retained capture has 18 such rows in three six-frame bursts; the
existing shot `$FFFF` and attached mode-twelve/seventeen checks stay intact.

The integration check distinguishes B34F entry animation from subsequent
AAB2 animation advancement before A7DA. Three entries keep the expected
upper/lower actions; nine continuations preserve selector, variant and
baseline velocities, decrement the timer by two, and retain attached
ownership/activity. Grounded continuations may retain ordinary animation;
airborne continuations require a valid close-finish upper action. Three
subsequent disruption releases each call 9D6E once, clear timer/flags,
install upper $17, and transfer ownership to a detached shot with $FFFF
activity. No production change was needed for either assertion correction.

The configured `build.ps1 -Test` gates passed across the initial full run
and retained CPU sections after focused assertion corrections. The complete
CPU prefix through scheduler checks, due-shot loop, score/inbound/camera/
attachment section, and final pass/sustained/image/static section all passed
with the same executable, ROM, and pack. The final trace has 69 shot starts
and releases, 2,847 exact pass frames, 142 automatic unlocks, and one pass
interruption/recovery. All five RGB anchors remain unchanged. The attachment
projection checks 17,733 stable rows using authoritative `+$2A/+$2C` rather
than delayed display mirrors, and preserves actor-relative Z for the late
mode-twelve/thirteen parent attachments. The mode-twelve initialization check
also proves an intervening odd-frame launch before accepting its next-due
`$0210->$01E0` jump and `$0948` `0->2` continuation.
An ignored instrumented copy identifies the intervening reset: actor 4
starts the shot, then actor 7 expires its invalid mode-ten receiver and
calls `$86:A613`. The final predicate requires that later receiver cancel;
five malformed transition variants are rejected.

Retained evidence is under ignored
`.analysis/cpu-mode-thirteen-20260906/manager-fullsuite/`; the trace SHA-256 is
`ac5abd618ac919a07897d910e75f8590a6b92642fdca1194d5b27f7c43a75afa`,
executable `1ef4e5ab37e56766cdddd43e9021bf7ae281d065d248ac0b0ac265f9132affaf`,
and pack `e22b8fde583246890ddd938f863e8cd1478ca1eea90d2c97baa7c153781857c9`.
Raw captures, rendered images, instrumented diagnostics, and packs are local
only. These regression results do not expand the native witness domain above.

`$86:B154-$B334` is the mode-fourteen special-receiver parent selected by
`$87:9BD3[14] -> $87:9C4E`. The production scheduler advances its animation
before the common `$85:963D` physics pass, runs ball/contact/role globals, and
only then dispatches the parent through `$87:9244`. A real pass catch at that
global boundary preserves mode 14, raises `$13E7` bit `$0010`, and defers the
parent to the following odd frame without repeating animation or physics.

Entry subtracts canonical `$C6=2` from actor `+$60`, saves the result in DP
`$AA`, then applies `DEC A/BPL`. This makes original timers `$8001` and `$8002`
continue with stored `$7FFF` and `$8000`, while `$8003`, `$0000`, `$0001`, and
`$0002` take their source-accurate terminal branches. Only the continuation
writes actor `+$60`; the terminal does not prewrite it. Owner terminals call
the existing adjacent
`$86:A9D0` then `$86:986D` chain. Lost-owner terminals and later invalid
relationships queue both animation channels to state 3 before clearing
`$1866`, restoring through `$86:9846`, and cancelling through `$86:A613`.

Grounded timers at least 40 retain state. An upper state outside `$18-$1E`
cancels both channels and installs the exact upper/lower queue layout: upper
state `+$66`, upper queue 24/cursor 0, lower state `$1F`, and lower queue word
`$86:B440[raw +$58]` plus 24/cursor 1. The lookup is an unaligned word read for
every safe raw byte offset 0 through 6; it is neither divided nor restricted
to even offsets. Valid grounded states below `$24` jump with VZ `$0270` for
selector zero or selector six with movement-facing `+$4E==3`, otherwise
`$0264`. Displayed facing `+$52` is not that branch input.

An owned airborne receiver is disrupted only when its upper state is invalid,
or when stored timer is at least `$12` and either wrapped signed velocity
difference from `+$BA/+$BC` lies outside inclusive `[-80,+80]`. The fallback
restores, cancels lower, installs upper `$17`, resolves and attaches, stores
integer shot origins, and enters the complete `$86:9D6E` launch. Retained
airborne selector zero uses raw `timer-8` byte offsets into `$86:A9B2`; other
selectors copy requested direction to movement-facing `+$4E`. Ownerless
retention requires `$0946` to name this receiver. Another owner additionally
requires that actor's `+$5E` to be mode 15. A self owner writes `$094E=30`,
`$0936=2`, clears actor `+$4A`, conditionally publishes `$1866`, and attaches
through `$87:B649/$B66A` without the ordinary `$87:AEC3` path.

Repeated controlled Mesen captures v5/v6 retain 51 genuine B154 entries, all
five exits, and the exact union of all 185 instruction starts. The owned-start
hash is `2ba100f987f03ddc4a7499dec07fcff87578ef71a202ee60ebfe90b726859d5f`.
Exit counts are B17D=4, B1B9=7, B242=8, B28F=3, and B334=29. Cases and paths
are byte-identical with hashes
`c58f19f007b91cc0d81b99c2f93f4a410522bc23b5431bfc936bbc74967a5527`
and `216e33722763b6ce4941048032ebf6ea1574cf33f5b5d2f5766eb13474881914`.
Raw-vector hashes are
`215155c4c085caf49af97dc84dd4e5bf915237e6279f9eed61af357fa4b2ba19`
and `c0d88a401cba7294e6c757c176d85814a2bf4e523e796e0eba49670889f24bfd`;
the raw vectors differ, while all 51 x 154 represented entry/exit words match.
The compact fixture hash is
`f26a26da6f6f750b3843caaf2d6ce1d519271b734b81e672784eb76cf5f1aec1`.

The native identity is slot/id 5 at actor `$39EB`, player `$446B`, and active
lineup `$4779=2`; pack roster `$AC:F636` maps it to host actor 5, context 1,
roster slot 2, and persistent index 14. Controller is `$FFFF`; period,
difficulty, and both context `+$08` fractions are fixed zero. The projection
includes all ten actor `+$5E` words, complete queues/cursors/locks/resources,
pose snapshots, roster/controller statistics, and attachment DP `$46/$47`.
Argument/address/arithmetic DP `$00/$8E/$AA/$B2` and other unrepresented
scratch are deliberately excluded. The Ghidra C export is not a snesrecomp
source, and no Bank-$86 recomp root is claimed.

Strict production replay passes all 51 x 154 comparisons, stale host-owner
poisoning, and twelve pre-replay fixture mutations. The production caller test
also covers canonical `+$60` with a poisoned compatibility mirror, the real
mode-11 special-pass transition, animation-before-common ordering, real
post-common acquisition deferral, owner-loss restore, and next-due work. The
frozen `fbeb3e0` source baseline fails 48 of 51 cases with 460 word differences
across 151 source-compatible fields; current production has zero. This bounded
claim does not establish whole-game native trajectory parity, controllers
zero through four, or nonzero period/difficulty/context-fraction behavior.

The mode-one frame-1000 image census remains a C regression anchor. The
preserved `fbeb3e0` executable and pack with the 620-byte shot-table resource
reproduce its former pixel-layer counts exactly. Against the frozen current
executable and pack with the 640-byte shot-table resource, the first
gameplay-row difference is frame 730, after actor 5 enters mode 14 at frame
728, and is confined there to its upper/lower animation accumulators changing
from 854 to 688. The reviewed current frame 1000 retains a coherent wide
court, players, ball, baskets, and crowd, with no HUD overlay. Its five updated
visible-layer counts do not claim native rendering parity. The ignored audit is
`.analysis/cpu-mode-fourteen-20260906/mode1-review/report.json`; baseline
executable/pack hashes are
`1ef4e5ab37e56766cdddd43e9021bf7ae281d065d248ac0b0ac265f9132affaf`
and `e22b8fde583246890ddd938f863e8cd1478ca1eea90d2c97baa7c153781857c9`,
and current hashes are
`97ed45622abeeee0f5ecb825b271e0db7844d75a0c9e769296051d48ab2dafaa`
and `378787f5a3b381cec616e63d34b05e3090518ba19b162e4412fbb0a6f182d504`.

The configured regression was completed in bounded segments against that
same frozen executable, pack, ROM, and retained 63,800-row trace. The front
gates, census, mode-one check, CPU prefix, complete due-shot loop,
score/rebound/camera checks, corrected attachment block, pass and stale-mode
checks, sustained analysis, RGB images, and static source checks all pass. The
due loop observes 77 starts and 77 releases. The pass section observes 2,731
exact pass frames, 134 automatic unlocks, and two pass interruptions with two
recoveries. Its source-accurate acquisition exception is limited to the
adjacent odd scheduler row and following restore; malformed intervening rows
remain rejected.

The attachment block retains all 15,494 stable samples whose native
`+$2A/+$2C` resource cache is valid. It classifies 363 excluded cache-invalid
samples, all and only grounded mode-fourteen state `0/31` with upper/lower
locks `0/1`; any other invalid signature fails. Frame 774 separately pins the
represented pre-parent fallback: moving owner state 5 selects upper resource
26, the locked alternate lower channel retains resource 2000, flags `$8004`
produce ball offset `(-5,-1,32)`, and late B154 reinstalls state `0/31` while
invalidating the end-of-frame cache. Sixty-nine other invalid-cache rows retain
an earlier special-shot hand point whose call-boundary inputs are absent from
end telemetry, so no native-parity claim is made for reconstructing those
points. The direct mode-fourteen vectors and production caller cases remain
the evidence for the late grounded queue and invalidation behavior.

All five current CPU RGB anchors match the reviewed hashes. Frame 600 is
unchanged; frames 1300, 3480, 6932, and 6954 show coherent court, players,
ball, baskets, crowd, and applicable HUD state. This visual review guards the
current C trajectory and does not add native instruction coverage. The
retained trace SHA-256 is
`b0f8ff3699320f03486e2674f07697efc9baac06fe6c3a41d46c90df2709e619`.
The exact corrected attachment source fragment and run log are retained under
ignored `.analysis/cpu-mode-fourteen-20260906/manager-fullsuite/`, with hashes
`5f6eba76f98aac64ee96c35816a2db030563602399def41e4df58871c9f10578`
and `0b2790c7a42895661095bc49cb5b285a035f010235d45b2199fece81bf49ab84`.

## Mode two defensive parent

`$86:F6CD-$F793` is the complete mode-two parent selected through the
`$87:9C21` wrapper (`JSL $86:F6CD; RTL`). It repairs locomotion base only when
both `$0946` and `$093E` are negative, subtracts the 30-Hz delta from actor
`+$60`, and either holds or reloads the decision timer with the player profile
and signed half-court adjustment. Recovery inhibit bypasses the decision
children. Accepted role pursuit goes to the shared jump gate; otherwise the
parent selects actor `+$74`, routes through the team context mode and signed
anchor tests, and uses the wrapped signed result of `CMP #3` on paired `+$92`
to choose `$86:E96F` or `$86:E7DC`.

The defensive target continuation follows native child order. A CPU defender
runs `$85:B3C9` for a stationary opponent or `$85:B402` for a moving opponent,
then `$85:A82C`, before copying `+$4E` to `+$50` and calling `$86:E3E1`.
Movement and target direction consume integer X/Y, and the acceleration gate
consumes integer Z; a nonzero fraction with integer Z zero remains grounded.
Human controller words skip those movement children. The close role-target
stop returns before copy/pose, and mode two never calls the shared `$86:E5AB`
finalizer. Its unrelated actor `+$64` cadence remains unchanged because this
parent does not call `$85:B4B9`. The final signed controller gate may call the
independently verified `$86:EC32`, then the parent always copies `+$50` back to
`+$4E`.

`tests/fixtures/cpu-mode-two-parent-witnesses.json` retains 43 controlled,
genuine `$87:9C21` dispatcher entries from repeated v17/v18 Mesen captures.
All 197 represented entry/exit words repeat exactly, all 81 parent instruction
starts occur, and the owned-start hash is
`3dcf5ee5a8c6d883acbaf57dd981962a08c51889b7ee52e75095aafb888a23b2`.
The six older leaf contracts account for 31 starts; this complete parent adds
50 without granting credit to called children. Aggregate observations include
20 normal, five weak and four role targets, two mode-three targets, ten jump
calls, 30 defensive-pose calls, eight stationary approaches, one predictive
approach, ten accelerations, one B37C animation reversal, and no E5AB call.

The captures write documented WRAM controls only before the natural dispatch.
CPU state, stack, flags, ROM and RNG are never written. No child result is
written while the parent is observed; the exit snapshot precedes restoration
of the original WRAM controls and state. Invalid v1/v2 runs that touched RNG
and later runs without immutable capture-source snapshots are excluded. The
v17/v18 raw hashes are
`f700bf22c2e2d7616ca2e064f2b3f3a2437cbcf467e7f650bcc9574a91441e69`
and `8f7a27063988d301c2579a196c8136c0d3a4ca67faca49d40ad2b5626546bb67`;
the compact fixture hash is
`6bdd4b38ff593f3ae736682355bf320f93eec657030c86718425c1ba1d10a02e`.
Immutable capture Lua, runner, recorder, case and path hashes are pinned by the
strict verifier.

The projection covers both team contexts, assigned opponents across slots
1-4 and 6-9, all parent-mutated actor words, RNG, `+$64`, pair/cache fields,
full signed controller `+$16`, full paired-role `+$92`, animation channels,
and B37C DP `$46/$47` publication. Natural controller records 0, 4 and `$FFFF`
are represented; `$7FFF/$8000` are explicit sign witnesses. Paired `+$92`
covers 0, 3, `$8000`, `$8002`, and `$8003`. Actor identities are immutable
metadata. `$092E` is excluded because the host has no storage for it and these
live-state-two paths preserve it. Period, difficulty and context anchor
fractions remain fixed zero. These boundaries do not establish a matching
whole-game trajectory or new child instruction coverage.

`tools/verify_cpu_mode_two_parent_vectors.py` pins fixture identity, fields,
cases, paths, child calls, source hashes and exact output shape, rejects a
partial binary record, replays production, and exercises a real
`nba_tipoff_update` rebound-state caller. The caller proves common movement
occurs once, the parent still runs during rebound state and changes velocity,
`+$64` is preserved, and full-word controller signs survive controller
bindings. The configured
107-call legacy normal-actor replay remains the independent leaf and jump
integration guard. The preserved `ba7d1d1` source/header/object baseline fails
all 41 initial complete-parent witnesses: 178 word differences include all 41
`+$64` results plus target, direction, velocity and cache fields. Current
production passes all 43 x 197 words. Focused replay used local pack SHA-256
`378787f5a3b381cec616e63d34b05e3090518ba19b162e4412fbb0a6f182d504`;
the verifier permits future compatible packs rather than pinning that whole
file. The source reference is the Ghidra Bank-$86 export and instruction
listing; no Bank-$86 snesrecomp source is claimed.

The C-only Tipoff frame-220 anchor changes with this source correction. The
preserved pre-change executable and pack reproduce RGB hash
`a021ed166d64811fade15c7f5c55ea8b20ca37522d96f7ab85020c3a52cd7c42`;
current production produces
`60a0315e4531c93d6595f3058537f2331799a4a09ef233b740fe5bae297ce193`.
Their debug-state summaries are identical, while 1,378 changed pixels are
bounded to `(95,35)-(235,98)`. Visual review confirms coherent defensive
spacing and poses near center-right with the court, center logo, ball, goals,
and player sprites intact. No HUD claim is made for this view. The ignored
comparison is `.analysis/cpu-mode-two-parent-20260906/frame220-review/` and
its report SHA-256 is
`43ffe6feaf76895847346efab5c38dff99c3910952d48cafaad301c02656d78f`.

The configured suite passes across frozen-input segments: all pre-CPU gates,
the initial CPU prefix, the corrected attachment block, and the unchanged CPU
tail. The original stop was an obsolete frame-774 coverage requirement, not
an attachment mismatch. The replacement derives resources from the pack and
a bounded same-owner animation history; it retains strict invalid-cache
classification and an exact semantic fallback witness. It checks 18,438 valid
attachments and 213 invalid-cache rows. The tail checks 2,714 exact pass frames,
138 automatic unlocks, sustained play, and all five reviewed RGB anchors.
The frame-1000 C-only layer census was also reviewed; native per-pixel rules
remain unchanged. Baseline image hashes were reproduced with the preserved
pre-change runtime. The first trace change is mode-two +$64 preservation at
frame 2; frame 181 then selects base 8 from post-acceleration VX 97 instead of
base 10. Retained evidence is in
`.analysis/cpu-mode-two-parent-20260906/manager-fullsuite/provenance.json`;
the 63,800-row trace SHA-256 is
`f9d371d153633607e2fab3b7aefa8ebb654f1c89d0ca191e279c80c0629d3a7c`.

The next workflow priority is human Exhibition controls. First complete the
human action parent `$84:E2AC-$E3E9`, concentrating on the missing
`$84:E2F2-$E3E9` continuation and its directly necessary pass/shot bindings.
The actor/control sweep `$87:9106-$92A4` unconditionally calls that parent,
so wiring the sweep first would expose incomplete actions. Its subsequent
integration must include the requester at `$87:9165-$91BF`, replace the
CPU-only `cpu_update_actor_behaviors` gate, and remove Tipoff's all-neutral
selection override only when the human branch is ready. A movement-only
subset would not establish complete human control. Each tested routine is
committed and pushed before the next implementation starts.
