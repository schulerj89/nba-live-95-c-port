# Human pass pose checkpoint independent audit

Verdict: **bounded source/native replay passes; v1 verifier rejected pending explicit binary-arithmetic CPU precondition**. No production wiring, whole initializer return, human enable or frozen source/evidence change was made.

Controller `build/human-pass-pose/freeze-v1.json` SHA256 `ee84e1455cb203a4d25273af22277925cba717ede116a51d53b0218daf4e1323` has 206 independently checked referenced identities. The reviewed C hash is `22a517945a1529a659528718ad97601e5ecfc0c3d3aced48ea5f11cba8f943c1`; header `cd2ae4a9b8f1b0954e9644ba2f3a696d2effa81a8dd4a768dd5dc353d05ca2b0`. Exact source copies and all audit outputs are under auditor `build/pass-pose-audit-v1`.

## Source and caller inspection

Actual ROM decoding confirms AEC3 resolves literal lower/upper phases through bank84, derives the mirror sign from facing0..2, uses ASL carry for direction indexing, applies the conditional upper resource+40 and preserves cached facing52 when facing=8. There is no phase modulo or fallback resource. Unsupported bank/table addresses are rejected by the bounded API.

B832 preserves the native order of sign extension followed by 16-bit negation. Flags28 sign toggles masks1/2. B8B1/B8B4 and B918/B91B produce the signed floor midpoint, not truncation toward zero. Point0 and nonzero-word point1 use distinct original arrays. B649 forces point0, preserves old ball X at0922 and writes integer ball X/Y only; the AF25..AF2D caller writes Z later. AF30 writes mode15 and OR6, then tests wrapped CMP80 N before setting live2. It stops before AF4D's stack restoration. Source comments preserve these original details and distinguish unwitnessed edges.

Thirteen reference ranges/data tables were independently read from original ROM, alongside nineteen reference artifacts, 67 source-tool identities and two completed Ghidra commands. `source-recheck.json` records the hashes. Key routines: AEC3..AF74 `4ac104b6ae4f3b9ad97e1db4d9a31a3440b3a1e5ae4ec010863317cb57535369`, B832..B952 `b37123e07b0395de2f11acc19434580405887842ed2c6769101fcebb7afae436`, B649..B669 `37fbb087270274f89bc91b54e411bc67de3eb86ce8ae089c6a5bb924ff5e8cad`. The probe also compares all49,536 bank84/attachment asset bytes directly with the original ROM before any replay.

Every C input is the actual component-entry sparse raw file plus immutable original ROM and candidate951f pack. Expected exit state is not passed to C. Each of the six modes compares all declared actor/ball, controller/context, profile/order, fourteen global and seven DP word families, including untouched fractions and velocities. CPU X/Y restoration and the AF30 actor-X binding are independently checked from metadata. CPU execution, flags and the AF4D stack epilogue are not implemented by this semantic module.

## Fresh validation

Fresh private module/probe compilation against immutable controller objects passes MSVC `/W4 /WX`. Both unchanged native captures pass: left168,096 plus right94,554 =262,650 compared values. There are25 calls each for resolve, offset, attach, prefix, commit and combined. The original manifests remain `ae07e3d2f3149d87a69dd0a9d14158f65b121f0cf42414db1bfe8b30ff7c2484` and `66774dfc7518f9c1a819d2021cda5eb6be8326f72d282cbd77ba7392949aa7c6`. All225 pose-boundary status records independently satisfy binary mode, 16-bit A and 16-bit X/Y.

Natural coverage remains point0, facing0..7 and actorZ0. Three calls arrive at genuine AF1D after an earlier AD3D stopping boundary; replaying their actual prestates does not implement that catch branch. Point1, facing8, extreme coordinates/bytes, nonzero Z and arbitrary phase edges have no natural witnesses here.

Independent `human_pass_pose_audit_probe.c` and `test_human_pass_pose_source_audit.py` pass889 controlled literal guards using a private synthetic table asset. These include point1/high-word selectors, all mirror masks, signed-byte -128→+128, negative midpoint floor, full-word coordinate wrap, literal phase/doubled-phase wrap, facing8 cache preservation, variant/lower-table selection, independent X/Y versus Z ownership, wrapped commit boundaries and invalid-address rejection without partial publication. They do not mutate the original pack or add native coverage. `source-contract.json` retains the result. Initial auditor build failures from redundant CRT macro and unsupported/constant assertion syntax are retained; only the audit harness/build invocation was corrected.

## Verifier-domain defect

Original verifier `d20feb6bb0cf5eedb545caff4646594e20143f4e66661d5bf1ad3ef6c91c649b` passes all73 implementer integrity cases. Independent `test_human_pose_protocol_audit.py` rejects ten additional malformed output/diagnostic cases but accepts setting CPU status D bit8 at `pose.entry` or `pose.offset.entry` after the original artifact hashes have been checked. The check at the pose dispatcher currently requires only `(cpu_ps & 0x30)==0`.

That admits an unsupported arithmetic prestate. AEC3 contains ADC; B832 contains ADC/SBC; no CLD/SED changes D in this source slice. As a minimal literal example at B8AF, lowerY9 plus upperY1 yields binary `$000A` and midpoint5, while decimal arithmetic yields `$0010` and midpoint8. The C API is intentionally binary and has no decimal-status input. This is an input-domain verification defect, not a reason to alter original arithmetic or implement a general CPU.

The independent report also records an accepted D-bit mutation at `pose.commit`. AF30 itself uses no ADC/SBC, so that case alone is not a numerical C counterexample; it violates the chosen complete caller-route precondition/unchanged-D provenance instead. Require D=0 for the bounded pose route and document it in a separate verifier revision. Preserve the original thirteen-case report (ten reject, three accepted), source/native fixtures and frozen C. No naturally executed mismatch was found and no original-game bug was normalized.
