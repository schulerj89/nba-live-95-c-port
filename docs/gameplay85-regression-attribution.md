# Gameplay85 C trajectory regression attribution

Full-suite v13 passes the preceding repaired gates, including unchanged
63,800-frame tip-flow endurance, then fails code14 for three historical C-only
digests. This document attributes each changed digest before updating it.
Independent review of this new baseline checkpoint remains pending.

The probe's three team pairs, seeds, 16,000 frames per pair, zero input,
controlled 43,200 clock/reset, hashing algorithm, render cadence, owner bounds,
2,400-frame dead-stall limit and all motion/scoring/resource guards are
unchanged. Only three expected C hashes and their explanatory comment change.
No original-ROM fixture or native expected word changes. These controlled
sustained-play runs are not full matches or native trajectory parity tests.

## Isolated source and configuration controls

All retained evidence is in root `build/gameplay85-attribution-v1`.
`checkpoints.py` extracts exact source, headers and manifest from the recorded
Git revision into a fresh directory and compiles every applicable production
source with MSVC `/W4 /WX`. It never writes to primary/main or reuses its
objects. `source-identities.json`, `compile.rsp`, batch, logs and private
objects remain beside each executable. Environment variables beginning NBA95
are removed only in each child process. All runs use the same candidate pack,
SHA256 `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

The private profile probe adds only an explicit pre-init legacy configuration
switch to the unchanged original endurance probe: Simulation, three minutes,
rules45/45 followed by eleven1 words. It leaves the hash function and semantic
guards unchanged. This is a labeled counterfactual, never a production default
or a substitute for normal-menu validation. Factory runs apply no overrides.

| Build / configuration | Scenario0 | Scenario1 | Scenario2 |
| --- | --- | --- | --- |
| Primary historical source / original defaults | `5c699ab7906a9264` | `e21e3fa411911e5f` | `310e2241d5021888` |
| 52c2899 with only historical tipoff / legacy | `5c699ab7906a9264` | `e21e3fa411911e5f` | `310e2241d5021888` |
| 52c2899 with only historical tipoff / factory | `17d6b3501ac8bb36` | `0625ec1c729a4706` | `aa1ef40c85d8a42b` |
| 52c2899 / legacy | `d730903256293d08` | `736e7b638bf5ff1e` | `74879cd0f462a359` |
| 52c2899 / factory | `4cae6c7d1e840f78` | `e1d6ab2561de623a` | `b1536ceaff7baeda` |
| Current objects with pre-layout tipoff/helper / factory | `4cae6c7d1e840f78` | `e1d6ab2561de623a` | `b1536ceaff7baeda` |
| Fresh complete 9f5b47c / factory | `be9fb0edcea0d524` | `2a8877c0056dc49d` | `fddf2d5e8ba68e5a` |

The primary control is a fresh probe linked to the read-only primary objects.
The stronger second control compiles all source from 52c2899 against that
revision's matching headers, replacing only `src/nba_tipoff.c` with exact
`caa91344879d3bdb408da86dc34683c6ed01ca2c` bytes. It independently reproduces all
three historical hashes and the original PASS. Matching headers matter:
attempting historical tipoff against today's headers fails compilation because
obsolete Mode11 fields were removed. That failed attempt is retained; no ABI
fields were invented or restored to make it pass.

The two-by-two 52c2899 matrix shows that the canonical team/rank changes and
the corrected factory configuration each affect these longer trajectories.
Unlike the prior frame1000 Mode1 check, configuration does matter here.
Fresh 52c2899 factory results exactly equal the later current-object build with
only the pre-layout helper and matching startup self-check restored. Later
controller/input work and the accepted wrapped F34F repair therefore do not
change any of these three complete hashes before the inbound correction.

Finally the fresh all-source 9f5b47c build reproduces all three full-suite v13
hashes. The sole intervening gameplay change from the pre-layout control is
the independently accepted C39C dispatcher correction and its startup check.
The helper selects the native edge constructor for layout1. It does not clamp
all targets: the original layout4 target404 remains preserved.

## Source justification and limits

`completion-mode1-attribution-audit.md` documents the original-ROM team-context
stores at `$86:DA8D-$DAAB`, sorted assignment rank stores at `$86:D789-$D7B7`,
and independently verified native first-court identity. Those source changes
are the tipoff difference in the 52c2899 controls; pause/substitution paths are
not exercised by these zero-input sustained-period fixtures.
`setup-config-native-contract.md` documents original factory Arcade/12-minute
configuration. Historical inbound-layout reports bind
the C39C correction to nine native cases/54 output words; the old helper fails
the five controlled layout1 cases.

The new default trajectories retain 23,725 total actor-motion frames,
12,210 camera changes, 553 possession changes, 39 score changes, 54 dead-ball
recoveries, 108,923 resource changes and 396 render changes. Passing these
guards establishes C regression continuity, not original full-game parity.

The old hash values, failed full-suite v13 log, original probe and every
counterfactual output remain preserved. Also retained: the first response-file
quoting failure, the initial factory invocation with an unsupported argument,
and the incompatible historical-source/current-header compile attempt. None
is counted as a successful comparison. Human play remains disabled; Rules
reentry still has 158 mismatches. No unexplained port defect is classified as
an original-game bug.
