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

The next bounded parent target is the mode-thirteen carried-ball close-finish
executor at `$86:A7DA-$A993`, implemented by `cpu_update_rom_layup`. Its directly
called terminal helper is `$86:A9D0-$AA69`; `$A994-$A9CF` is inline table data
and must not receive instruction coverage credit. Existing
evidence covers its `$86:B34F-$B624` initializer and aggregate product behavior,
but does not pin the executor's exact genuine-entry call set, owned instruction
starts and exits, child counts, or the ordering of its private planar/vertical
movement and repeated pose attachment against the production actor caller.
