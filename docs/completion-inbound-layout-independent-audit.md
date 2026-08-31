# Inbound layout freeze v2 independent audit

Verdict: **the bounded layout-1 production fix and its C-only Mode 1 attribution pass; the original verifier is rejected**. Frozen source/evidence remains unchanged. New verifier acceptance requires a separate revision and review.

Reviewed freeze `completion-owner/build/inbound-layout-freeze-v2.json` is SHA256 `68b5f25b531467118c81af554a976a97138a45425a52e8550b4189a3cf8aa1d8`. All 363 identities were read and checked independently. Exact source snapshots and fresh results are in auditor `build/inbound-layout-audit-v1`. Reviewed gameplay C is `0f05e141e553e5c989360c82aaf4ca64ab74c34d02a94dc0be29a7bfa1d87aca`; tipoff C is `c7f87cf5757f97545431ffaa4c03992d380e7fcdd7659a8f8bb5bda45c3f5990`.

## Source repair

Actual canonical ROM `$85:C37D..C5BF` hashes to `a4cbc2fcc01a754a213e79317a2ed8c70ed8cf5ea82c3ee50a3ebfb9db983680`. The bytes at `$85:C39C` are `C9 02 00 D0 03 4C 9E C4 10 03 4C 0B C5`: CMP #2, BNE, the equality jump, BPL, then JMP C50B. For layout 1, CMP produces `$FFFF`; BNE preserves N, so BPL does not branch. C50B is the correct edge path. The old C erroneously grouped layout 1 with layout 4 at C450.

The changed helper moves layout 1 into the existing edge branch and keeps layout 4 in the original ball-X/endline branch. The source comment identifies the PCs and previous stall consequence. The tipoff runtime caller still passes integer coordinate words, including truncated live ball X for layout 4; its production behavior is otherwise unchanged. Its startup guard now checks both layout-1 edge coordinates and the existing layout-4 fractional-X projection. This is a port repair, not a repair to the original game. Case 6 explicitly retains original layout 4's X=404/Y=-224 result: no blanket clamp was added.

This review covers the canonical layout states and the declared controlled coordinate cases. It does not establish behavior for arbitrary otherwise unreachable 16-bit layout encodings, every coordinate extreme, full CPU return state or elapsed timing. The earlier wrapped-direction helper correction is unchanged.

## Independent fresh checks

Fresh `/W4 /WX` helper/probe compilation produced `compiled/target.exe` SHA256 `69c9f7648e0b4a4436f989e7cb2c986f3dbcd624712b4fb0fbc0dcb9d58b9f8f`; a separately fresh pre-repair helper probe is `2def191e41324ceea46124882e88ebbf057840cfea3589dd9955dc5c2ddb1f02`. Both link only the required immutable frozen dependencies. Frozen checkpoint headers were not included in its 363 entries; the independently recorded fresh header inputs are in `header-input-identities.json`. Headers used by these probes match the earlier independently accepted controller checkpoint. The unchanged cancellation fixture also matches that checkpoint. Initial private compilation failed on these omitted local include copies; those failures are retained and are not reported as successful builds.

All nine unchanged captures replay through the fresh corrected probe: 54 target/direction/play/request/RNG words pass. The independently fresh old helper fails exactly cases 1–5, all layout 1; natural case 0, both layout-4 cases and the negative-layout control pass unchanged. Actual recorded PC lists contain C50B once for cases 1–5/8, C450 once for cases 6/7 and neither for the natural layout-3 call. Raw ROM disassembly and per-case manifest hashes are in `source-recheck.json`.

Case 0 is an unmodified genuine call at frame 5374/court 984. The other eight cases modify exactly the declared eight WRAM words at that same genuine entry, leaving CPU/clock and ROM untouched. Full before/entry/exit bytes establish the injection boundary. This is controlled original-ROM source evidence, not proof that the injected prestates occur naturally. The prior timeout capture rejects on process exit; the earlier case-2 isolation failure remains incomplete and rejects for missing completion. Neither was reused.

A freshly compiled endurance probe passes the unchanged 63,800-frame journey, period 1 and 12,026 post-restart live frames. Its four controlled cancellation/recovery projections and attached whole-update binding pass. The output explicitly reports zero naturally observed cancellations/recoveries; controlled coverage must not be relabeled natural. No full-suite completion is claimed.

## C-only Mode 1 attribution

Both frozen old/new application binaries were rerun independently to frame 1000 with the same canonical ROM and candidate951f asset pack. They reproduce the old census BG1/BG2/BG3/OBJ/backdrop `750/46939/5641/1362/2652` and new `0/46422/5641/1298/3983`. The old binary's private relink substitutes only the pre-layout helper and its matching startup guard among the frozen production objects; the renderer and assets are unchanged.

Independent fresh gameplay telemetry is identical through frame 505. At 506 precisely seven values change: selected play, RNG, inbound target X/Y/direction, and actor 7's target X/Y. The original target 40/224 becomes -394/89. `independent-attribution.json` records the exact differences. Both old and new Mode 1 scripts pass against their respective binaries. The entire test tail beginning with pixel counts is byte-identical, preserving all 57,344 per-pixel rank/palette/accounting assertions. Only the four changed census numbers and explanatory comments differ. This supports the narrow C-trajectory baseline refresh; it is not native frame, HUD or PPU parity.

## Original verifier rejection

Original `verify_inbound_layout.py` SHA256 `0acdfd28a5d232368ac614f39827d86976ab4e9d188716a610f897ed85293d47` passes its existing 33 tests but accepts all nine additional cases in `independent-integrity.json`, exercised by `tools/test_inbound_layout_integrity_audit.py`:

- Extra contradictory manifest field and floating-point controlled addresses.
- Wrong isolation method and an extra unattested isolation field.
- Capture, runner and isolation-helper source identities redirected to byte-identical aliases rather than the actual copied route paths.
- An extra opposite dispatcher destination C450 in the layout-1 PC list.
- Correct-looking stdout accompanied by nonempty failure stderr.

The path cases expose absent association between the executed copied artifacts and the pinned source declarations; source content identity alone does not enforce that association. Repair exact manifest/isolation schemas, scalar types, expected copied paths and their artifact identities, unique source-defined dispatcher destination, and the stdout/stderr protocol. These mutations use parsed views and new small alias files only; original captures and frozen source remain unchanged. The nine failures and original verifier are preserved even if a later revision passes.
