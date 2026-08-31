# Human B switch child checkpoint

This adds eleven new files after the frozen controller19 and human-dispatch10
checkpoints. It implements the complete persistent-state effects of
`84:E141-E2AB`, reached from the B-button switch branch at `84:E2EB`.
It does not enable normal human gameplay, modify the source manifest, or wire
the new module into the production scheduler. No prior checkpoint file or
binary is changed. Independent review and root integration remain required.

The private worktree is
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-controllers`.
Evidence paths below are relative to its `build/human-control-action` directory.

## Native contract retained

`nba_human_switch_control` reads the current actor, native team scan bounds,
published controller pointer090C, controller direction, context controller
count, and neutral anchor0910. The actor positions and assignments come from
the complete native entry snapshot. No owner is injected or reassigned.

- Count>=5 returns immediately, before the other switch inputs are consumed.
- A direction selects among uncontrolled teammates, excluding the current
  actor, relative to the current actor's position. The original `85:F34F`
  helper supplies direction and weighted distance. Each octant of angular
  difference costs64. Equal scores replace the previous candidate: later
  candidates win.
- Neutral direction selects the nearest eligible actor to pointer0910,
  including the current actor. Equal scores do not replace the earlier one.
  The inline native distance uses the sign of a wrapped16-bit subtraction
  when choosing major/minor components; it is not replaced by unsigned max.
- A transfer copies the source actor's controller assignment to the selected
  actor, clears only the source assignment, and writes the selected local
  index plus the source actor's group to the published controller record.
  This is the direct `E208-E21B` transfer, not allocator `86:BC9B` round-robin
  selection. Ball owner, receiver, processed latches, counts and cursors stay
  unchanged.

The original does not initialize selected-pointer scratch A6 or candidate
index C2 before scanning. If no candidate improves the initial1600 score,
`E1F6` consumes incoming A6 and, on a transfer, incoming C2. The new API carries
both values explicitly. It preserves mapped native behavior and rejects an
unmapped host target; it does not substitute the owner/current actor. This
source-derived quirk is commented at its native PCs but was not reached by
the natural captures. Likewise, the wrapped-magnitude extreme is implemented
from the instruction stream, not established by these ordinary court positions.

The module models persistent gameplay changes and the restored caller words,
not a CPU register file, stack or instruction interpreter. Volatile scratch
AA/AE/B2/92 and return CPU flags/registers are outside this API. Production
caller integration must preserve its own subsequent ordering. The existing
direction helper preserves native X internally; this child does not call the
A82C movement routine or the frozen human10 motion stage.

## Source evidence and captures

Original ROM `F:\Games\SNES\NBA Live 95 (USA).sfc`, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Original Mesen executable SHA256
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.

The fresh source reference at `reference-v3/manifest.json` has SHA256
`fcc4b1814a33e5133a6d59e7606828f3e2c19c60551b6ef42e9c9bf628fefee9`.
It records original bank bytes, bounded recompiler emission, independently
seeded Ghidra65816 listings, tool-source hashes, exact commands and logs.
The complete switch byte range `84:E141-E2AC` (exclusive end) hashes to
`c725d8ffa282684386f10cb3d42a64d094f95b8b8a654922f123fdb9ad797411`.
The reference also includes `85:F347-F3BA` and its16-byte direction map.
The first listing omitted the E231 neutral entry; version2 added it but did
not include F347's zero-distance return in the bounded listing. Both incomplete
reference attempts remain preserved. Version3 includes both. No native output
or C behavior was changed to conceal a mismatch.

Each new capture uses a fresh process, private portable executable/settings,
empty private save directory and explicit subprocess environment. It enters
through Title and the normal menus, chooses left or right, then supplies
ordinary directions and B presses over2400 court frames. The original decides
whether B passes or switches. There are no CPU/RAM writes, ROM patches or
loaded savestates. The executed Lua/runner/helper, observed Lua home, initial
and persisted settings, final saves and raw entry/return snapshots are hashed.

| Capture | Whole switch calls | Transfer / retain | Compared values | Manifest SHA256 |
| --- | ---: | ---: | ---: | --- |
| `selection0-v1` |14|13 /1|23,968|`a91b123ad729fab9cb42b7f980310fe64689f076990c978ffb81aa8a47cd31e9`|
| `selection2-v1` |26|19 /7|44,512|`817c0d2b7543b013cca8ae35edf2b4188f35e786acc58895a2157f932d4cfdc5`|

Both final reports pass with zero mismatches or tolerances. Together they
exercise all eight decoded directions plus neutral, with tip, inbound and
live states. The right route contains a native equal-score replacement at
entry95/event98. There are no frame crossings within these40 calls; this is
an observed limit, not a filter. The captures also record16 left and9 right
native pass entries for route context. Those pass bodies are not replayed or
claimed as implemented by this checkpoint.

The probe consumes only the native entry. It compares the resulting persistent
state against the untouched native return: all1408 actor/ball words,160
controller words,128 context words,15 explicitly preserved global/caller
words and one observed return route per call. The raw sparse input map remains
0000..00FF,0500..09FF,1600..18FF,3400..49FF (7936 bytes). Missing raw bytes or
unmapped inputs fail instead of being implicitly zero-filled.

The final warning-free private build links frozen objects read-only. Probe
SHA256 is `7d273ed8875c6916dfd25e0a2848adf3fc5600385cb9b19ee239c0546bd29502`.
The initial executable is retained separately so earlier report hashes remain
reproducible. Final reports are `selection0-final-v1.json` and
`selection2-final-v1.json`; build output is `build-final-v1.log`.

## Evidence gate and remaining work

The new verifier is separate from the frozen earlier verifiers. It pins the
executed Lua/runner/helper version, original ROM/Mesen, exact command and
three process environment fields, runner frame limits and selection. It
requires all artifact identities, complete paired events, exact JSON types,
nonzero directional/neutral calls and complete parsed C results. It checks
actual private settings/save/home hashes before calling the shared isolation
helper on a copy, then rejects any metadata changed by that helper. The initial
settings recipe is checked structurally with exact scalar types.

`verifier-mutations-final-v1/report.json` rejects35 metadata and C-output
mutations against the actual unchanged right capture. These include missing
source/artifact keys, float/bool schema, forged settings/save/home, missing
command/environment, wrong runner bounds, zero-call completion, and partial
or malformed typed C output. This is a bounded rejection suite, not an
exhaustive verifier-security claim or additional game coverage.

The one-controller journeys do not cover count>=5, protection of another
human's assigned actor, no-improvement/stale A6, neutral equal-distance ties
as a distinct native witness, or extreme wrapped coordinates. The source
implements those branches, but native acceptance is limited to the captured
inputs. Invalid pointers/directions remain explicitly outside the host domain.

The next B action remains `84:DF7A`: it seeds0944 with090E|$10, has a separate
directional/neutral receiver-selection policy, and calls `86:AB2D` with native
receiver identity/pointer. Its directional octant penalty is256, unlike the
switch child's64. Its neutral branch uses actor+8C preference and `85:F1C1`.
The existing port has grounded pass initialization, but that does not by
itself establish this human caller, all pass families, or their return effects.
This checkpoint does not route B-pass requests through a fabricated receiver.

The requester stage, other human action buttons, processed marking, behavior
dispatch/return effects, physical commit, multipad/pause/free-throw lifecycle
and production human enable gate still remain. Root is separately repairing
the independently discovered original carried-X A82C quirk in copied human10
sources; that issue and its old immutable evidence are not altered here.

Reproduce with `tools/build_human_switch_probe.ps1`, then
`tools/verify_human_switch.py --capture <capture> --probe <private probe>
--rom <original ROM> --output <NEW report>`.
`tools/capture_human_switch.py` accepts a NEW output directory, selection0/2,
frames400..3000, and explicit original ROM/Mesen paths. The attested captures
above use2400. Root owns integration, commits and any source-manifest change.
