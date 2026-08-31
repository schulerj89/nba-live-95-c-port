# Independent Mode 1 frame-1000 attribution

**PASS for refreshing the five C trajectory winner counts, with the limits below.**
The failed census in `tools/test_snes_mode1.py` comes from the preserved WIP
team-context/rank correction in `src/nba_tipoff.c`. It is not caused by a Mode 1
priority change, the new headless driver, the candidate asset pack, or the
factory configuration at this particular frame. This audit did not edit the
test or its baseline and does not establish native gameplay/pixel parity.

All experiments ran in `.analysis/worktrees/completion-auditor`, using fresh
production objects from the independently accepted headless freeze-v4 build.
No parent objects, executable or native fixtures were modified. The original
ROM and all referenced native artifacts were present.

## Controlled source/configuration experiment

`tools/mode1_attribution_probe.c` enters the real `NBA_STATE_TIPOFF` initializer,
advances 1,000 zero-input ticks, renders once, and records full pixel and logical
traces. An explicit `legacy` pre-init switch restores the historical C
Simulation/three-minute rules; `factory` leaves the corrected factory state
alone. This injection is only a controlled experiment, not a normal-menu test.

The historical `nba_tipoff.c` was extracted byte-exact from
`caa91344879d3bdb408da86dc34683c6ed01ca2c`, compiled against current compatible
headers, and linked with the same current objects as the control except for
`nba_tipoff.obj`. The existing `NbaTipoff` layout did not change. The current
session layout was used by both binaries. This avoids swapping incompatible
old object layouts. Separate primary A2 executable runs corroborate the old
source result without that controlled relink.

| Source/executable | Config / pack | BG1 | BG2 | BG3 | OBJ | Backdrop |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Old tipoff, current other objects | Legacy / candidate | 2176 | 38703 | 5641 | 2573 | 8251 |
| Old tipoff, current other objects | Factory / candidate | 2176 | 38703 | 5641 | 2573 | 8251 |
| Current tipoff, current other objects | Legacy / candidate | 750 | 46939 | 5641 | 1362 | 2652 |
| Current tipoff, current other objects | Factory / candidate | 750 | 46939 | 5641 | 1362 | 2652 |
| Untouched primary A2 executable | Historical defaults / primary pack | 2176 | 38703 | 5641 | 2573 | 8251 |
| Untouched primary A2 executable | Historical defaults / candidate pack | 2176 | 38703 | 5641 | 2573 | 8251 |
| Fresh auditor production CLI | Factory / candidate | 750 | 46939 | 5641 | 1362 | 2652 |

Every old-cohort full JSONL trace is byte-identical, as is every current-cohort
trace; the BMPs also match within each cohort. These are complete-output
comparisons, not merely equal winner totals. All seven full 57,344-pixel traces
pass the existing per-pixel priority ladder, palette/color constraints, exact
pixel accounting, and indexed/direct-presence assertions without changes.
Only the five historical trajectory totals differ.

The compositor, renderer and asset-loader sources equal their historical
versions after CRLF normalization. `nba_player_lab.c` differs only in its
comment explaining that sorted offsets now feed gameplay rank publication.
The tipoff diff changes team identity publication/consumption and sorted rank
publication, along with their dependent self-test prestates and pause mapping.
The zero-input experiment does not pause.

## Source and native ownership

The original ROM bytes, Ghidra `PublishTeamIdentitySlice`, and recomp
`bank_86_D73E_M0X0` agree on these boundaries:

- `$86:DA8D-$DAAB`: with normal Exhibition `$15C3=0`, `$16B1` publishes to
  context0 `$46EB`, and `$16B3` to context1 `$476B`. Native WRAM shows those
  values are home/right and visitor/left respectively. Current
  `src/nba_tipoff.c:64` implements this publication before ratings and
  appearance initialization (`:8274`).
- `$86:D789-$D7B7`: the sorted actor offsets resolve through `$87:9C7B`, then
  receive descending rank4..0 in actor `+$92`; `$D97A/$DA07` call this for both
  teams. `src/nba_tipoff.c:84` consumes the translated sort. It no longer uses
  the roster-position category for that field.
- Current live consumers use the published team context, including pass
  ratings (`:804`), paired actor data (`:999`), and later strategy/rating
  reads. The paired actor's native `+$92` reaches its consumer at `:1016`.

The fresh `first_court_identity_probe` again matches all128 projected identity
words in two natural native fixtures: `selection2-v3` and
`selection2-teams2-pause-v2`. The verifier rechecks their actual raw files and
source identities, supplies only UI team/quarter inputs to C, and derives
expected identity from native WRAM and original-ROM ratings. Historical runner
and Lua-home provenance gaps remain disclosed in its reports.

Both projections were also compared directly with the stronger native
`team-context/build/native-hud-default-v2` and `native-hud-alternate-v1` WRAM:
another128 exact words. This supplemental comparison rechecks snapshot hashes,
classification and exit0; it does not repeat the older complete754-artifact
provenance audit recorded in `team-context-independent-audit.md`. None of these
identity projections proves 1,000-frame native motion or camera equivalence.

An additional explicit counterfactual holds corrected teams/appearance/ratings
but writes the old roster-position values into actor `+$92` immediately after
initialization. It produces a third full trace: BG1=1059, BG2=43327, BG3=5641,
OBJ=3418, backdrop=3899. Restoring only that old rank field therefore does not
restore the old trajectory; rank publication and the remaining team-context
work both contribute. Its current-rank control reproduces the production CLI
trace exactly. Both additional traces pass all unchanged per-pixel assertions.
The old-rank intervention is intentionally non-native and must not become a
production fix or parity oracle.

At frame0 both cohorts start at camera(-128,-124), RNG37190, owner-1. By frame1000
the old cohort has camera(-451,-54), RNG7105, owner6; the corrected cohort has
camera(-358,-122), RNG9713, owner2. The different camera and actors explain why
large court/backdrop census changes can occur without changing layer priority.
I inspected the paired BMPs: the court resources remain recognizable and the
camera/player composition differs. The stale WEST/ORLANDO HUD panel remains an
explicit unresolved consumer, not an approved native quirk.

## Recommendation and retained evidence

Root may update only the five C census expectations to BG1=750, BG2=46939,
BG3=5641, OBJ=1362, backdrop=2652, preserving every per-pixel assertion and
recording the causal evidence above. Keep the historical totals in this audit
and the old captures. Rename the misleading final `native-input winner counts`
message to describe a C trajectory regression. Do not rebaseline any raw native
fixture or infer approval for other C trajectory goldens; each needs its own
scope and evidence. Configuration may affect later gameplay even though it
does not affect these frame1000 pixel outputs.

Evidence under the auditor worktree:

- `build/mode1-attribution-v1/matrix.json`: all seven exact commands, executable
  and output hashes, source hashes, per-pixel checks; original logs/traces/BMPs,
  old source/object/probe, original probe source and compile command alongside.
- `build/mode1-attribution-roles-v1/report.json`: two rank interventions, exact
  commands, source/executable/output identities.
- `build/mode1-identity-audit-v1/{default,alternate}.json`: fresh native identity
  gate results and disclosed limitations; `supplemental.json` records source
  lineage, original ROM opcode spans, fresh WRAM corroboration, logical
  checkpoints and the two additional full-pixel assertion passes.

Key SHA256 identities (full output and additional source hashes are in those
reports):

| Item | SHA256 |
| --- | --- |
| Original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| Candidate pack | `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df` |
| Current tipoff source | `1d02593536fb0494290002bf20e712b677fe4854e0a4c4bcf59d4adaa5f4ca75` |
| Current compositor source | `6bcbd81f92cbbb98e654b23c16a579ee5e914bc540bb3208c23afe08825711bb` |
| Unchanged census test | `ca2604cbda17bf874ac0da61bca5d56852e5525966d211e62d53d2f46f5f14ac` |
| Final controlled probe source | `5d49a770e5a14480a65a51c88d17b0a8c1469f0b17971918573d932653a629a9` |
| Old-cohort full pixel JSONL | `483ebeaa4cb99cd24ac86a6b743c3b5f2b23a16693bc3f6218ac7a508756ec7c` |
| Current-cohort full pixel JSONL | `fa5d69bb6385037ff3a17826410a946a27dfc8b2739e6040627980ef9122a271` |

No unexplained divergence was classified as an original-game bug. The older
team/rank C shortcuts conflict with original stores; preserving native bugs
does not require retaining those host shortcuts. This is a bounded C regression
attribution and initializer check, not completion of team context, HUD,
controllers, gameplay timing, graphics parity or the game.
