# Exhibition team-context and actor-assignment correction

This bounded change fixes the production initializer's team identities, the
consumers of those identities, and actor `+$92` publication. It does not enable
human control or claim equivalent full initialization, scheduling, gameplay,
or HUD presentation. Worktree base: `2723af6`; integration must preserve the
new-match reset subsequently accepted on main in `e1bc0d4`.

## Native ownership and implementation

Normal Exhibition `$86:DA8D-$DAAB` publishes the right/home UI team to native
context `$46EB` (context0/group0), and left/visitor to `$476B`
(context1/group5). The C initializer previously published the opposite team
IDs while retaining native actor groups, anchors and uniform slots. That
bound the wrong rosters/ratings and drew Knicks in home white, Rockets in
visitor colors for the default New York-at-Houston selection.

`publish_exhibition_team_ids` is the UI boundary. `team_id_for_context` reads
the published native context thereafter. The correction applies to fatigue
initialization, active appearance, jersey/number rendering, movement/shot/pass/
contact/jump ratings, CPU decision profiles, substitutions, and home-court
selection. Controlled leaf-test initializers now explicitly publish their
declared UI team choices; production consumers do not silently fall back to
UI fields when a fixture omits this prestate.

The existing appearance sorter already returned the correct sorted actor
offsets, but its production caller discarded them. Native `$86:D789-$D7B5`
writes rank4..0 through those offsets at `$D7A8`; `$D97A/$DA07` process both
contexts. C previously substituted roster-position categories for actor
`+$92`. The initializer and substitution rebinder now publish the translated
sort's results after preparing both teams. The underlying pure sort is
unchanged.

Pause selection now explicitly maps the frontend's left0/right1 choice to
native context1/context0. Its host `selected_side` stores a context index,
not native `$08D2`: the latter and timeout `$4933/$4935` carry group0 or5.
Native `$86:818D-$81D2` reads the actual requesting controller record; natural
left and right controller captures establish those values. The current
one-controller pause API still derives ownership from the saved frontend
choice. Neutral/multiple-controller ownership remains a later task.

Session scores, timeouts, fouls, roster statistics and lineup arrays remain
in native context order. Existing telemetry names `score_left_raw` and
`score_right_raw` are historical protocol names for native `$4711` home and
`$4791` visitor, respectively; their numeric protocol was not swapped.

## Independent native initializer projection

`tools/verify_first_court_identity.py` reads original-ROM active roster
pointers from native WRAM `$3449+4*actor`, then reads stamina directly from
that ROM record's `+$35`. It compares the C fatigue table's stored rating,
not a newly fetched rating using an expected team formula. The other native
words come directly from first-court WRAM captured at `$87:A47A`, after
observing the real `$86:E208` controller initializer.

The C probe receives only the three native UI inputs (visitor team, home
team, quarter length), calls `nba_tipoff_init`, and prints eight fixed arrays:
team identities2, anchors2, actor groups10, active roster slots10, assignment
ranks10, appearance variants10, height variants10, stored stamina ratings10.
The denominator is **64 identity words per case**, not 64% of initialization.
RNG, controller records, velocities, scheduler phase, clocks, resources,
camera, ball, audio and rendering are excluded.

| Natural capture | UI visitor/home | Native context0/context1 | Before | After |
| --- | --- | --- | --- | --- |
| `selection2-v3` | New York17/Houston9 | Houston9/New York17 | 13 differing words | 64/64 exact |
| `selection2-teams2-pause-v2` | Orlando18/Indiana10 | Indiana10/Orlando18 | 21 differing words | 64/64 exact |

Raw captures live in main `.analysis/controller-ownership-20260830/`.
First-court WRAM SHA-256 values are
`174ba3ced580e4810c6dfb2894774808b7567cd20dcf80919cd40819d68ec732`
and `da82a3d39d95a4e16a462441beca90994a4364d35cb825219c60d9b4d4353673`.
Both use ROM `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`
and Mesen `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Inputs are natural menu presses with no WRAM/CPU/ROM injection. Private
settings disable random power-on RAM and isolate saves. The historical
captures did not log Lua's observed home; the first did not preserve an
immutable runner copy. Those provenance limitations are reported, not erased.

The verifier rejects duplicate JSON keys, incomplete projections, booleans
masquerading as integers, wrong dimensions, invalid native domains,
non-permutation assignment ranks, missing/changed raw artifacts, wrong ROM/
Mesen, changed executed script/settings, and missing observed native
boundaries. Subprocess failure and timeout fail the run. Protocol corruption
tests are explicitly synthetic tests of the verifier, not native game proof.

## Other verification and limits

- Unchanged native active-appearance vector: one captured call, no mismatch.
- Active appearance runtime: 300 records across29 teams passed C consistency
  checks. Foul-out/substitution runtime: four teams, both native contexts,
  appearance rebuild and no-bench atomicity passed. These are C regressions.
- Timeout/resume runtime: both contexts, native group0/5 publication, stamina
  restoration and freeze behavior passed controlled C checks.
- Jump runtime: two launches over1810 calls, contact160 and possession182;
  a C regression, not a native trajectory comparison.
- Actual before/after C screenshots and fresh native court30 were inspected.
  Houston home white/New York visitor blue now agree. The frames are not
  scheduler aligned, so full-pixel equality is not claimed.

The current packed BG3 score panel still contains WEST/ORLANDO labels and is
drawn continuously. Native `$87:B99A` clears that map at initialization;
normal `$83:CC10` schedules `$D0AD/$D157/$D1B1/$D1FD/$D2E0` after a basket.
Replacing that stale display with the native publisher is the next dependent
change. The fresh natural neutral-controller capture
`build/native-hud-default-v1` observes the first basket at court970 and
name publication at974; it retains full WRAM/VRAM/CGRAM at routine boundaries
plus61 consecutive RGB/PPU frames. No full context/HUD subsystem completion
claim is made while this consumer remains wrong.

CPU-versus-human remains disabled pending complete controller allocator,
input-bit conversion, human dispatch and transfer wiring. Season/Playoffs
team swapping, full scheduler/RNG synchronization, lifecycle presentation,
gameplay audio and complete endurance/differential acceptance remain outside
this bounded correction. No expected C trajectory hashes were rebased.
