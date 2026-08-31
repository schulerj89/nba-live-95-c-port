# Independent bounded controller audit — 2026-08-31

**PASS for the bounded composite checkpoint:** the unchanged frozen controller
C implementation plus root's final verifier SHA256
`eb44afe38217dfcd7dfdfb2432434dda87d5ef439e76ee75eaa84b4778b0e0db`.
Source, raw native leaf, controlled caller and45 independent integrity cases
pass. The original verifier's18 failures and first repair's11 additional route
failures remain documented below with their immutable failed reports.
Human gameplay remains intentionally disabled. Neither this audit nor the
passing leaf probes establishes normal human C/native journey parity.

## Frozen material and independent build

Owner worktree:
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-controllers`.
Auditor worktree:
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-auditor`.
All auditor paths below are relative to the auditor worktree; owner native
captures remain in their original directories and were not copied or changed.

The exact 19-file `build/controller-contract/controller-checkpoint.patch`
SHA256 is `22b914c0276ec41a540f432a46ef18aa27f44bf97e9c268bba7332cdf7d82b69`.
Its `final-v1/manifest.json` SHA256 is
`63c1f337303e23b08a6644794a23b5fee1e4c762ef0997c7f22b443c033e7184`.
The base is `52c28996cdf693e0ae45aef714b47f698abd3ee1`.

The auditor exported that base, applied the exact patch in
`build/controller-audit-v1/source`, checked its normalized content against the
19 frozen files, and restored the attested original raw line endings. Every
final raw file hash equals the manifest. See `source-identities.json`.
The initial nested `git apply` invocation silently skipped files; identity
checking caught the base-only result before any candidate comparison. That
rejected build remains in `source/build-rejected-base` and
`rejected-base-build.log`. Only the subsequent full fresh rebuild in
`source/build`, logged in `fresh-build.log`, is counted here.

| Identity | SHA256 |
| --- | --- |
| Frozen controller module | `c3f12d2f41bb13c978207f091af3256e638090da7aa5e621e5e573a02287322e` |
| Frozen controller header | `8d530f64d69ba2da7b9bb467034213d111a37f221b964a17b84ce35c6b252c80` |
| Frozen tipoff integration | `793d46aeeb0553252f57def1b92369c6ed0ad2e8bcb5d3d00a8dc355c91f4a87` |
| Frozen Player Setup implementation | `b72a18c82f0fa09dda85b4504cd6cc22f46c8c78ba5d9fab6c990230c531f604` |
| Frozen verifier, rejected | `cf0cf9bc5c34350b4cce5029fb314fd8719ad46c10e53750a9c6d557008390a2` |
| Fresh auditor frontend executable | `6e94135b0c9914af17f43f8d1224d0efa915337cbe3825624416a2090988c53d` |
| Fresh auditor controller leaf probe | `4416e0d3258616445f9e475592d1fa1be0b0ce7a75b80cb4c51a2d93f779fec2` |

No new human-dispatch files from the owner's subsequent work are in this source
snapshot. No production source, ROM, asset, expected vector or gameplay golden
was changed by this review.

## Source and real caller review

The original ROM was read directly from
`F:\Games\SNES\NBA Live 95 (USA).sfc`, 1,572,864 bytes, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
All seven named original-byte ranges and all five bank binaries in the owner's
`build/controller-contract/reference-v3` agree byte-for-byte with that ROM;
see `build/controller-audit-v1/original-rom-reference.json`. The actual bank81,
85,86,87 listings, bank84 action closure, and bounded generated bank86 C were
inspected. This is not approval based solely on the implementation summary.

| Native contract | C implementation/caller inspected | Conclusion and boundary |
| --- | --- | --- |
| `86:E208-E24B`, `E24C-E389` | `nba_controller_initialize/allocate`; `nba_tipoff.c:86`, `:8516` | Five 64-byte records, previous selections, actor assignments, context counts, untouched tails and cursors follow source. Normal startup deliberately allocates effective all-neutral selections, preserving saved UI choices. |
| `85:EF3A-EFEC` | `nba_controller_publish_input`; `nba_tipoff.c:107`, `:7120` | Full held/previous/XOR/newly-pressed fields, both direction tables, active-roster stamina, boost and Traveling state effects match the bounded routine. Current actor sweep still lacks the native human action/movement caller. |
| `86:BC9B-BD1E` | `nba_controller_transfer`; `nba_tipoff.c:97`, `:7466` | Team-count gate, matching-group scan, cursor increment/wrap, old-owner clear. Direct ROM bytes at `87:9CDE` contain the shooter comparison and `JSL 86:BC9B` at `9CE6`, before `JSL 87:A15C`. |
| `86:D25A-D349` | `nba_controller_acquire`; `nba_tipoff.c:6310` | Previous controller publication and designated receiver vs ordinary transfer branches precede the existing acquisition continuation. The native write witnesses distinguish `D2BF` from ordinary `D31C`. |
| `87:9075-9086` | `nba_controller_begin_sweep`; `nba_tipoff.c:7120` | Exactly five processed latches clear. Native `9087-908D` also resets requesters `095E/0960`; that later scope remains missing and is disclosed. |
| `81:A7AB-A843`, `B748-B75E`, `A2B8` | `nba_player_setup.c:21`, `:110` | One-step assigned/neutral/assigned selection, left priority, preserved high bit, assigned-only L/R toggle before arrows, and neutral glyph row clear are source-backed. Full native menu polling/confirmation/repeat/transition is not accepted by this bounded review. |

Actor `+16` remains authoritative at gameplay mutation boundaries. The helper
copies it into/out of the controller module; Mode11, free throw and inbound
consumers use the same records/counts. Host-to-native button conversion now
occurs before `pad_held_raw` publication, and the remaining consumers use native
masks or an explicit inverse conversion. Removed owner-following assignment
wipes do not silently reappear elsewhere in these integration paths.

Normal startup still sets `cpu_vs_cpu=true` (`nba_tipoff.c:8368`), calls the
initializer with `[1,1,1,1,1]`, and has no new assignment or flag path that enables
human play. The first intentional difference from a selected-human native
journey is therefore already the initializer result. The current per-pad
publisher plus CPU dispatcher cannot be described as completed human gameplay.

The implementation comments preserve the source-confirmed high-bit selection
asymmetry, unchanged-selection override behavior, designated receiver's lack
of a team check or cursor update, BC9B-vs-D25A cursor difference, and wrapped
CMP-sign test. Corrupt host indices/no-matching-record cases return failure
instead of emulating arbitrary native reads or an unbounded scan; the document
correctly treats these as representation limits, not fixed original bugs.

## Fresh comparisons and independent evidence validation

The auditor freshly built and ran the exact frozen leaf probe against the four
original capture directories. Each comparison reads complete original native
entry WRAM into C and compares projected output with the separately captured
native exit WRAM; it does not import expected poststate into C.

| Immutable capture | Fresh compared words | Calls: init/alloc/transfer/input/acquire | Input NMI crossings |
| --- | ---: | --- | ---: |
| `selection0-v1` | 25,292 | 1/1/0/137/0 | 0 |
| `selection1-v2` | 716 | 1/1/2/0/0 | 0 |
| `selection2-v4` | 121,916 | 1/1/2/660/6 | 5 |
| `selection2-live-pass-v1` | 122,096 | 1/1/2/660/7 | 3 |

All **270,020 word comparisons pass**, without tolerances or skipped captured
calls. Reports are in `build/controller-audit-v1/<capture>.json`. The older left
capture did not yet instrument every transfer/acquisition; its zero counts
must not be reported as proof of absence. Neutral has no input publication by
design. The selected-mode empty-result bug below is a separate verifier defect.

Independent evidence checks bypassed the weak verifier checks and verified all
five source identities, the complete inventory and hashes of **3,055 artifacts**,
actual initial and persisted private settings, fixed no-patch/zero-RAM/single-pad
settings, recorded post-settings hashes, observed private Lua directories,
final save identities, and route environment values. All pass in
`build/controller-audit-v1/evidence-integrity.json`. The historical left runner
has no court-frame option or manifest field: its immutable Lua exits at court400.
Later captures have explicit court-frame options; no schema was retroactively
rewritten. Captured Lua uses pad input and read callbacks, not RAM/CPU/ROM writes.

Both assigned native label rows contain the same 320 original bytes as pack
asset257 at VRAM byte8370 (SHA256
`6b919a6491067db86c4c06d0a09bcaa9696700e9c06c578d8e332866b975092e`);
neutral's native WRAM row is 320 zeros (SHA256
`7b6436b0c98f62380866d9432c2af0ee08ce16a171bda6951aecd95ee1307d61`).
Raw metadata independently confirms row pointer934E, count0140, tile0037 and
base4000. Native controller OAM begins at x40/109/174; neutral omits the arrow.
This validates the bounded data publication and placement, not full-image
palette/timing parity. The unchanged candidate pack hash is
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

Fresh controlled runtime, timeout/resume, natural/delayed C tip possession and
foul-out substitution probes pass. Existing native gates pass 61 Mode11 calls,
22 catch-core plus17 tip-bridge calls, and two64-word initializer identities.
See `remaining-probes.json` and its per-probe logs. Those older identity
fixtures retain their original historical isolation caveats. Root separately
identified inverse home/visitor adapters in legacy probes; the frozen Mode11
probe passed its61 cases but still needs the root's mechanical adapter merge.
That is not permission to alter expected values or native captures.

The independent `tools/controller_contract_audit_probe.c` adds **200 checks**:
untouched record tails/cursors, five-pad assignment, override retention,
high-bit reallocation, designated cross-team transfer with zero team count,
no cursor advance, designated vetoes, distinct corrupt-cursor behavior,
processed-only clear, all32 direction-table entries read from the original ROM,
stamina boundary, free-throw boost retention, and wrapped-sign Traveling gates.
These controlled C cases distinguish plausible incorrect translations; they
are not claims that all such branches were naturally exercised. Probe source
SHA256 is `109bc063de11b07d6dd600211193407f9bf0d28a3a45a13a3fa290a06447e231`.

## Verifier rejection and required repair

`tools/test_controller_contract_integrity.py` SHA256
`17249bb8b02ad185e0794596604cbe2c49d36ec2976745e85d9807edcef1a435`
mutates manifest/probe responses only in memory. Its clean baseline reaches716
actual word comparisons. `build/controller-audit-v1/integrity-v1.json` records
all34 outcomes; **18 fail** because invalid input is accepted.

- `verify_controller_contract.py:57-64`: loops validate only dictionary entries
  present. Removing ownership JSONL, completion, executed Lua, or initial
  settings from artifacts still passes. Removing capture, runner or isolation
  helper source identities also passes. Require exact source sets and complete
  core artifact coverage, including every consumed file.
- `:49`, `:65`, and manifest consumers: schema2.0, selectionTrue,
  court_framesFalse, post_settings_verified1 and float artifact sizes pass.
  Require exact types and bounded domains; bool is not a numeric word/count.
- `:68`: the capture-time `mesen_portable.verify` helper overwrites the supplied
  post-settings hash and final-save inventory. A zeroed attested post hash or
  invented final save passes. An empty declared settings dictionary also
  passes. Reverification must compare actual data with preserved attestations
  and required settings, without repairing metadata in memory.
- Route environment selection can disagree with the manifest and still pass.
  Validate executed runner/script version and route options together, honoring
  the explicitly documented historical variants without mutating fixtures.
- `:139-144`: `modes=['input']` on neutral returns passed with0words/0calls.
  Require nonempty executed comparison coverage; omitted historical modes may
  remain explicitly reported missing, not converted into fake coverage.
- `:116`: filters stdout down to lines starting with `{`; added unframed text
  is ignored. Require exactly one complete JSON response and defined framing.

The existing exact projection field/list/type/value tests do reject corrupted
final words in all five arrays, missing/extra fields, bool/float words,
truncated lists and duplicate JSON responses. The defect is not that every
comparison is vacuous; evidence/schema/coverage enforcement is incomplete.

The final repaired verifier passes this unchanged independent tool and all
four immutable native captures with the same word and call counts, as recorded
below. No source-only bounded pass grants full human gameplay, natural
C routing, full UI screenshots, pause request ownership, multipad lifecycle,
or full-game/scheduler parity.

## First repair re-review, retained as v2

Root's verifier-only freeze `controller-verifier-freeze-v2.json` contains
verifier SHA256 `cce6bd55db0326ef663ef3119a83a56156acb34d81856d36a17fb19a777764b5`.
The auditor copied it to `build/controller-audit-v2` without modifying the
19-file controller snapshot. The unchanged34-case suite passes. A fresh left
native replay also passes the same25,292 words and explicitly reports its two
unwitnessed modes. The exact legacy fixed400 source-pair exception is justified
by the original runner/Lua; the exact input-mode ROM loader line is legitimate
probe framing. Persisted settings checks no longer repair a bad attestation.

However, `tools/test_controller_capture_route_integrity.py` finds11 additional
accepted invalid declarations on the unchanged neutral capture, each still
reaching716 comparisons: enabled or omitted team-variant setting, enabled or
omitted pause route, court999999 with matching environment despite the executed
runner's400..2000 range, absent command arguments, wrong command ROM path,
extra command option, unknown source entry, unsupported live-pass declaration,
and undeclared live-pass environment. See
`build/controller-audit-v2/route-integrity.json`. These concern the preserved
capture provenance, not a newly observed production or native routine failure.
The full four-case replay was repeated after the following narrow repair.

## Final composite acceptance

The auditor copied root's `build/controller-verifier-freeze-v3.json` and exact
verifier to `build/controller-audit-v3`, leaving the earlier snapshots intact.
The final verifier requires the five exact source identities, exact executed
command and version-specific environment keys, team-variant0, pause-1,
court400..2000, and a typed live-pass option supported by the attested runner.
Its historical fixed400 exception and legitimate input ROM-loader framing
remain bounded to their actual source contracts. Unknown runners and the11
invented route declarations are rejected. No capture metadata is rewritten.

Both unchanged independent test tools pass:34 protocol/evidence cases and11
route/command cases. The route test source SHA256 is
`d36960845a8f9dd23dcd40be76fcfd15767b511c950543b8a35b17369183dfab`.
Fresh final native reports in `build/controller-audit-v3/<capture>.json` retain
25,292/716/121,916/122,096 words, exactly the original call counts, and all
five/three input frame crossings. Zero-observation modes are explicitly
reported; a request with no comparisons fails. Every root freeze artifact
hash and all19 frozen source identities were independently checked again.

This accepts the bounded controller data/module/caller checkpoint and repaired
verification protocol. It does not enable human gameplay, approve the later
human-dispatch source, claim complete menu polling or pause ownership, approve
full UI palette/timing parity, or establish a naturally aligned C/native game.
The separate root mechanical Mode11 fixture adapter merge must retain its
expected outputs and receive its own combined regression rerun.
