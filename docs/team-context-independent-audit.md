# Independent audit: Exhibition team identity and actor ranks

**PASS for the bounded identity correction. FAIL for complete team-context,
controller, HUD or gameplay completion.** This is an independent review of
patch `770438eba7b2184e6838222ba0c9d0216a242e502ce938a4d482437c83fa7113`,
not approval of the implementer's later HUD changes.

The auditor created a detached checkout at
`2723af610aab0ec63263a6449fa6a161a155f974`, applied the frozen seven-file patch,
copied its four hash-attested probe/verifier/document files, and compiled all
production objects afresh. No implementer object files or HUD changes were
used. The eleven files match the frozen source after checkout line-ending
normalization. Root integration must preserve the later `nba_session_begin_match`
change; applying this old-base session file wholesale would lose it.

Audit worktree: `.analysis/worktrees/team-context-audit-20260830`.
Raw audit outputs and build command:
`.analysis/team-context-transition-auditor-20260830/`.
Fresh probe SHA256:
`7d3d61ce2066e3b35741fc61c675b7c079164e7e2b97a65d6ac44fb3e2957764`.
The audit used the frozen version30 asset pack
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`
appropriate for this source base, not a newer pack with unrelated HUD assets.

## Native ownership and real callers

I read the original ROM bytes, Ghidra's controller and gameplay85 bank86
listings, and recomp `PublishTeamIdentitySlice_M0X0` and
`bank_86_D73E_M0X0`. The latter is retained in
`.analysis/shot-state-recomp-20260827/generated/bank86_v2.c`.
The original ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The independently recorded opcode spans and hashes are in
`raw-original-and-fresh-corpus.json`.

- `$86:DA85` calls the real controller initializer `$86:E208`. At `$DA8D`,
  normal Exhibition `$15C3=0` takes the branch that copies `$16B1` into
  `$46EB` and `$16B3` into `$476B`. All inspected native first-court states
  contain `$16B1=home/right`, `$16B3=visitor/left`, while the UI working
  words `$16FB/$16FD` retain visitor/home order. The new publication helper
  implements that boundary. The other mode/swap branch is not implemented
  or approved by this review.
- `$86:D789-$D7B7` walks the sorted actor offsets, resolves them through
  `$87:9C7B`, and stores descending rank4..0 in actor `+$92`. Calls at
  `$D97A/$DA07` publish both groups. I also checked the native five-byte
  lists at `$4734/$47B4` against every target actor's raw `+$92`, for both
  fresh team pairs. The C helper consumes the existing translated sort's
  offsets after both team records are prepared, including substitution
  rebuilds. It does not replace this rank with roster position.
- The ordinary C journey reaches `nba_tipoff_init` through
  `nba_game_enter_state(NBA_STATE_TIPOFF)` after Player Introduction.
  Context IDs are published before stamina initialization, appearance,
  matchup construction and rendering. The independent native gate calls
  this initializer directly and therefore does not prove the preceding C
  menu/intro journey or equivalent scheduler boundaries.
- `$86:818D-$81D2` resolves the requesting controller and publishes its
  group to `$08D2`. Read-only examination of the natural left/right pause
  captures confirms selection0 -> group5 and selection2 -> group0. The
  host context array index and native group are correctly distinguished.
  Current C pause ownership still uses one saved frontend choice, not a
  complete controller allocator.

## Exact comparisons and regression classification

Both first-court fixtures were replayed against the fresh auditor-built C
probe, with only visitor/home/quarter input values supplied to C. No native
expected arrays were passed to C. Results:

| Native natural journey | UI visitor/home | Exact identity words |
| --- | --- | --- |
| `selection2-v3` | New York17 / Houston9 | 64/64 |
| `selection2-teams2-pause-v2` | Orlando18 / Indiana10 | 64/64 |

The denominator is two team IDs, two anchors, and ten each of actor group,
active roster, assignment rank, appearance variant, height variant and
stored stamina rating. It excludes all other initialization state. Stamina
expectations were independently read from the original ROM using the
native WRAM active-roster pointers, while C reports its already-stored
fatigue ratings. This is not an implementation-derived fixture.

The historical captures lack Lua's observed home, and the first lacks an
immutable runner copy. I independently corroborated all128 words using the
fresh natural neutral-controller captures `native-hud-default-v2` and
`native-hud-alternate-v1`, without using their HUD implementation. Every
identity word matches the historical native fixtures and fresh C. I checked
all754 artifact hashes, original ROM and portable Mesen identities, initial
and persisted settings, no initial saves, actual Lua home under the private
portable directory, explicit exit0, the completion marker, and the single
observed `$87:A47A` first-court event. Thus the missing historical home
record remains disclosed, while these identity values have additional
independent natural evidence with the stronger provenance.

Fresh native first-court snapshot SHA256 values:

- Default: `b6fe71fb0d50a54f5b995594e78bcda748d5ea4d3d3494604005115819bc7766`.
- Alternate: `22ffee864f9edbe2c2079b029bac99b3ae5cad2fdbc4dc45b54105cada43360d`.

Additional fresh auditor runs passed: active-appearance consistency300 actor
records across29 teams; foul-out/substitution four teams and both contexts,
appearance rebuild and no-bench atomicity; timeout/resume both contexts,
native group publication, freeze and stamina restoration. These remain
controlled C regression tests. Their changes correct declared fixture
prestate or native ordering; no assertions were removed, tolerances widened
or trajectory/image hashes rebased.

All seven verifier protocol tests passed, including corruption of each of
the64 projected words. A separate auditor script rejected24 manifest
classification, source identity, initialization and numeric-domain mutations.
The verifier reads and checks the actual raw source files and subprocess
status with a timeout; it is not a C-versus-C parity gate.

## Downstream consumer review

I searched all production source for UI team fields and every roster/rating
lookup. The changed gameplay consumers consistently use published context
identity for jump, movement, pass, shot/range/free-throw, contact and CPU
decision ratings; fatigue; active appearance and matchup preparation;
substitution selection/rebinding; sprite/head/number diagnostics; uniforms;
and court resource selection. CPU strategy already consumed the context
team field directly and now receives its corrected value. Camera and court
geometry consume unchanged native anchors/groups. Scores, fouls, lineup
and statistics arrays remain in native context order. Remaining UI
left/right reads in team selection, player setup and introductions express
frontend presentation choices and do not rebind live actor identities.

The context-ID helper has no fallback to UI fields when a test omits the
required state. Controlled leaf fixtures now establish that prestate.
Substitution prepares a complete actor set and rank publication before its
atomic commit. These are source/caller and C regression checks, not native
trajectory equivalence for each consumer.

I viewed fresh C30 and native30 captures for both team pairs. Home white and
visitor colors, roster appearance and court identities agree visually.
The frames are not scheduler aligned: ball state and motion differ. No
full-pixel or timing claim is made from those images. The stale
WEST/ORLANDO panel remains visible in C for both pairs, while native has no
panel at that point; this is an explicit failing dependent consumer.

## Acceptance limits and follow-up

The bounded patch is suitable for integration after preserving newer main
changes and running the integrated build. It does not complete the entire
native initializer, season/playoff team swapping, human control,
multicontroller/pause ownership, HUD publication, lifecycle presentation,
audio, scheduler/RNG alignment or full gameplay equivalence. No percentage
of game completion follows from128 exact projected words.

One nonsemantic documentation follow-up: `nba_player_lab.c` still says its
sorted lists are kept as evidence/diagnostics. Sorted actor offsets now
also feed gameplay rank publication; the implementer was asked to correct
that comment during integration. No runtime failure in the bounded identity
change was found.
