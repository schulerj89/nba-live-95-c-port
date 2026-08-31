# Independent CPU image / inbound oracle attribution audit

Accepted only for five attributed C RGB anchors and three source-backed
conditions in the Python inbound-target helper. The frozen CPU test still
rejects the incomplete period restart at frame49412. No full CPU-suite,
native gameplay trajectory, period formation, HUD, human enabling, Rules
reentry or whole-game acceptance follows from this checkpoint.

Owner `build/cpu-oracle-freeze-v1.json` SHA256
`3bc921054e89d4af5a0c177a045b1ce44b57ee9fac44a7ea5f1bddaf4dbcfccb` and all
114 declared identities independently rehash. The authoritative test is the
frozen `build/cpu-oracle-attribution-v1/review/tools/test_cpu_gameplay.py`,
SHA256 `27651a7daaa650dbed89020674ddcd5e259e04dd7a72850e40b660f49a35da53`.
Auditor evidence is `build/cpu-oracle-audit-v1`.

The frozen before-test is byte-identical to the independently accepted
actor-trace/period checkpoint. An independent AST comparison permits exactly
five EXPECTED_RGB values and these three condition substitutions; all other
executable Python nodes are identical:

| Previous condition | Corrected condition |
| --- | --- |
| `layout < 0` | `layout < 0 or layout == 1` |
| `layout in (1, 2, 3, 4)` | `layout in (2, 3, 4)` |
| `layout in (1, 4)` | `layout == 4` |

These match the previously accepted original `$85:C39C` CMP/BNE/BPL routing:
layout1 takes C50B, while layout4 retains C450. Running the isolated old and
new Python helpers on all nine retained native input/expected records
reproduces exactly five old layout1 failures and nine new matches. Layout4
`(404,-224,0)` remains unchanged. These are the accepted one natural plus
eight declared controlled native calls, not nine unmodified natural journeys.
No native input/expected fixture or production source changes here.

Independently read all 25 frozen BMP arrays and executable identities, then
reran all five cohorts at600/1300/3480/6932/6954 into new private output. Every
new RGB array matches its corresponding frozen array byte-for-byte. For the
old-tipoff/canonical-tipoff cohorts, this replay uses the auditor's earlier
fresh complete source builds from the accepted tipoff-image audit; for
current it uses the auditor's fresh 40-source actor-trace build. Primary and
pre-layout use the separately frozen executables. The latter's selected
tipoff/helper object substitution and matched headers are already covered by
the accepted closure and actor-trace source freezes; this review does not
relabel that retained executable as a fresh build.

All five old-tipoff arrays equal the primary executable's old arrays. All
five canonical-tipoff arrays equal the pre-layout control. The five current
arrays equal the proposed new hashes, with no tolerance or image masking.
The prior source-control audits attribute the first change to canonical
team/rank initialization and the next to the already accepted layout1 repair.
This is a C-only trajectory/render regression attribution, not native image
parity or a new PPU approval.

Viewed all five current images plus the old600 comparison. Court, sprites
and colors remain rendered; the static WEST/ORLANDO scoreboard and visible
HUD/player overlap remain explicit port presentation gaps. They are not
called original-game bugs or silently accepted as live native HUD behavior.

Re-evaluated the exact frozen shot and inbound AST sections against the
unchanged63,800-row trajectory. Shot checks retain77 starts/76 releases.
The corrected inbound helper passes its previous frame506 barrier but the
section still fails at49412 with `resumed ready inbound state changed`.
This failure and the original source/period attribution are retained; no
assertion was removed to obtain a whole-suite success.

The first audit render wrapper used a wrong extra `compiled` directory in
the paths to the two previously built tipoff CLIs. It stopped after the five
primary renders without changing evidence. `replay_remaining.py` records the
correct existing paths and completes the other20 renders; earlier output and
the failed wrapper remain. The AST/freeze checks had already passed. Scoped
section execution uses the frozen test with copied, hash-recorded dependency
helpers from the preceding accepted actor audit, never a later mutable test.

The five new C hashes are retained in `renders.json`; source/AST/native-case
evidence is in `source-contract.json`; the still-failing period section is in
`sections/results.json`. No new tests of original behavior, native fixtures,
source files, timings or tolerances were rewritten for this acceptance.
