# Bounded controller integration

This integrates the independently reviewed controller checkpoint into the
owner branch. Human gameplay remains deliberately disabled: normal startup
still allocates effective neutral selections, and the native human action
caller is incomplete. No main-branch or desktop replacement is implied.

The original19-file freeze is
`completion-controllers/build/controller-contract/final-v1/manifest.json`,
SHA25663c1f337303e23b08a6644794a23b5fee1e4c762ef0997c7f22b443c033e7184.
Seventeen files were copied exactly after confirming the applied patch differed
only in line endings. `build/controller-source-copy-v1.json` records that check.
The two explicit exceptions are the independently repaired verifier and the
Mode11 adapter, which also retains the accepted native home/visitor mapping.
No native fixture or expected projection changed in that merge.

The verifier's original frozen version is rejected. The accepted replacement
is described in `controller-verifier-integrity-repair.md` and independently
reviewed in `completion-controller-independent-audit.md`. It rejects missing
or contradictory capture metadata, empty comparison coverage, malformed
responses, and undeclared routes/commands. Its supported immutable runner
contracts are explicit; adding a new runner revision requires review. The
current capture runner uses pinned LF checkout bytes to retain its identity.

The only owner production adapter change is in the CLI: `--player-setup-left`
now sends two separate Left taps, traversing RIGHT(2), NEUTRAL(1), LEFT(0).
The mandatory release frame remains intact. Debug output uses the actual
masked native selection and can report neutral. Tests at201/202/203 frames
verify neutral, neutral, left, respectively. This does not enable human play.

The historical left image expected x42, while the original OAM has first
controller tile x40 and arrow x41. The unchanged natural left capture's
`player_after.oam` entries51..55 are (x,y):41/111,40/110,56/110,72/110,72/118.
The accepted renderer uses that native position. A private counterfactual
changes only the renderer offset from134 back to132; its complete image
reproduces the old primary executable and hash3dfda176... exactly. The actual
correction changes318pixels solely in x40..78/y110..125. The replacement C
regression hash is3ed5198441075776e92641754bfd2302ed2782050e5889270693052988c8ad23.
See `build/player-setup-attribution-v2/report.json` for exact commands and
binary/image hashes. Failed initial compilation attempts and the original
failing Player Setup test log remain preserved. No full native UI claim follows
from this positional correction; palette and transition differences remain.

Fresh combined verification uses `build/nba95_controller_owner_v2.exe` and
newly linked probes after a complete owner build:

- Four unchanged natural captures:270020 compared native words, retaining all
  recorded frame crossings. `build/controller-combined-v2-*.json`.
- All45 independent verifier integrity cases and200 controlled original-ROM
  direction/quirk checks pass.
- Mode11:61 native calls; acquisition:22 catch-core and17 tip-bridge calls,
  all with unchanged expected projections.
- Controller runtime including4096 button conversions, timeout/resume,
  Player Setup handoff/images and two-tap CLI sequencing pass.
- Existing input-only replay:730 native configuration checkpoints and57391
  input frames, plus its automatic journeys and rejection checks, pass.

The runtime and ROM-direction/quirk probes are included in `build.ps1 -Test`.
External raw native captures remain separately attested evidence, rather than
being silently required from a particular developer filesystem by that suite.
The complete suite, initial/repeated transition scheduler and whole-game
acceptance remain incomplete. This checkpoint preserves source-observed
quirks, including wrapped sign comparisons and designated-transfer behavior;
unexplained port differences are not labeled original bugs.
