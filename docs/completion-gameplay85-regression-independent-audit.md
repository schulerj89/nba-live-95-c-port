# Gameplay85 C regression attribution independent audit

Verdict: **PASS for the three C-only expected-digest updates**. No original-ROM expected output changes, native trajectory parity, whole-game completion or human enablement are accepted by this review.

Root `build/gameplay85-regression-freeze-v1.json` SHA256 is `503676b958a134e1f23a1a09a2193b51ab2260c74b7e4cedd89baac2c05a5356`. All 627 file identities were independently checked. The exact original and rebaselined probes differ only in comments and `expected[]`; removing those sections leaves identical code. Team pairs, seeds, zero input, 16,000 frames per pair, forced long-clock renewal, hash projection, render cadence, owner bounds, stall threshold and aggregate motion/scoring/resource assertions are unchanged. The original expected hashes and failed attempts remain preserved.

## Independently rebuilt controls

Auditor `build/gameplay85-audit-v1` contains private exact source/header/manifest copies, build responses/logs, objects, probes and fresh run logs. Every copied source identity was checked against its actual Git object, including the single historical tipoff substitution. Three sets were freshly compiled with MSVC `/W4 /WX`, with 37, 37 and 38 applicable production sources respectively. No shared objects or build outputs were changed. All child processes remove NBA95 environment overrides and use the same candidate pack `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

| Independently rebuilt source/configuration | Scenario 0 | Scenario 1 | Scenario 2 |
| --- | --- | --- | --- |
| 52c2899 + exact caa9134 tipoff / legacy | `5c699ab7906a9264` | `e21e3fa411911e5f` | `310e2241d5021888` |
| Same / factory | `17d6b3501ac8bb36` | `0625ec1c729a4706` | `aa1ef40c85d8a42b` |
| Complete 52c2899 / legacy | `d730903256293d08` | `736e7b638bf5ff1e` | `74879cd0f462a359` |
| Complete 52c2899 / factory | `4cae6c7d1e840f78` | `e1d6ab2561de623a` | `b1536ceaff7baeda` |
| 9f5b47c with only pre-layout 1c3c60f helper/tipoff / factory | `4cae6c7d1e840f78` | `e1d6ab2561de623a` | `b1536ceaff7baeda` |
| Complete 9f5b47c / factory | `be9fb0edcea0d524` | `2a8877c0056dc49d` | `fddf2d5e8ba68e5a` |

The legacy counterfactual sets only the declared pre-initialization Simulation/three-minute and rule words. The factory path makes no override. Both source and configuration affect these longer runs; configuration is not incidental as it was in the earlier frame-1000 Mode1 attribution.

Every freshly reproduced scenario line matches the root's corresponding output, including motion, camera, possession, pass, shot, scoring, recovery, resource and render counters. Historical tipoff plus legacy configuration passes the original three expected hashes. Other old-oracle controls intentionally return digest code 14; their failures are retained, not called successful end-to-end gates.

For the additional pre-layout control, the auditor independently extracted only `nba_gameplay_ai.c` and `nba_tipoff.c` from 1c3c60f, compiled both against the matched 9f5b47c headers, and linked all remaining freshly built 9f5b47c objects. Inspection of those two source diffs confirms only the layout1 branch correction and associated self-test/comment changes intervene. This control reproduces all three 52c2899 factory hashes and counters exactly. It retains the accepted wrapped target-direction helper; that repair is not reverted.

Finally, the auditor freshly compiled the rebaselined probe against its complete 9f5b47c objects. All 48,000 frames pass with the new expected values and every original assertion. Totals match: 23,725 motion frames, 12,210 camera changes, 553 possession changes, 39 score changes, 54 dead recoveries, 108,923 resource changes and 396 render changes.

## Source attribution and limits

The historical/current tipoff control changes canonical home/right and visitor/left contexts, profile/resource consumers and source-sorted assignment ranks. These original stores were already independently reviewed at `$86:DA8D..DAAB` and `$86:D789..D7B7`; this audit does not use a new hash to justify them. The factory defaults likewise retain the separately reviewed native contract. The final changed branch is the accepted `$85:C39C` CMP/BPL layout1 correction, which was checked against nine genuine native entries/54 words; original layout4 behavior remains preserved.

The tests are sustained, controlled C trajectories within one period. They force the long clock and reseed after initialization; they do not validate a naturally played complete match. Passing hashes establish regression continuity after source-backed corrections. Normal human play, Rules reentry and unrelated pending paths remain outside this result. All original/rejected artifacts remain intact.
