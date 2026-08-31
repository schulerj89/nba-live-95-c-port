# Human aligned-pass checkpoint independent audit

Verdict for original freeze: **bounded source/native replay passes; original diagnostic protocol rejected**. The separate v2 verifier acceptance is recorded in `completion-human-pass-action-aligned-verifier-v2-independent-audit.md`. Original failures, sources and captures remain unchanged. No normal human gameplay or whole-initializer acceptance is implied.

Controller `build/human-pass-aligned/freeze-v1.json` SHA256 is `c3401c907d34347a950e47a9176553e3437e74cf5eb094efc9a6b326b09a597b`. All 146 referenced identities were checked, including dependency sources, frozen link objects, private objects, original assets/ROM, references, capture artifacts and prior freezes. Exact copies and fresh outputs are under auditor `build/pass-aligned-audit-v1`.

Reviewed C SHA256 is `abedbe82183ea5ec51efa35304f119e43e90624f807c02140bc37051db79742f`; header is `d22d8747c4810ed546d68271bb9c0aa7b7241ba5ea03c994935d42ebc4e76f9e`.

## Actual source and native boundary

Actual ROM `$85:F473..F5E3` hashes to `444f5bd46b33477ca8c9c4ed11537a400ba07e51daee081c7c9947b14bbc81de`; `$86:AD0E..AF2F` to `6483ac62a37745c25ffc2321c79fc6c59a6d541561092d08ca7e27a4367db694`. Eleven ranges, fourteen Ghidra/recompiler artifacts, 67 tool-source hashes and three completed command records were independently checked against the original ROM. The canonical ROM hash is `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`. Details and independently unpacked raw court-270 observations are in `source-recheck.json`.

AD0E uses wrapped subtraction N for the live-state, band and receiver-anchor comparisons, then the full word OR of receiver Z and vertical velocity. Its matching early path returns before AD3D. AE19/AE1C perform consecutive SBCs without a second SEC or mask: the first borrow affects the second full-word result. Nonzero relative direction bypasses this computation. Selection then preserves unsigned selector/direction branches, receiver mode 14 bypass, the lane result, the original `$07F6 & $30` test, live/layout comparisons and profile/distance/anchor gates.

F473 expands endpoints by 24 using wrapped subtraction signs. Its XOR tests include the lower endpoint and exclude the upper endpoint for ordinary ranges. It follows actor `+14` through the odd-address list `$34D1..34E9`; it skips the receiver's list cell, the ball and same-team actors. Forward X failure immediately restarts backward; backward X failure immediately returns clear. Y failure continues. These behaviors, including unusual results on unsorted or wrapped inputs, are preserved and explicitly commented. They are source contracts, not claims that all such inputs naturally occur.

At AEDD, family is published before the negative-relative branch to AF30. Near `$2C` becomes `$2F`; distant stationary non-inbound requests stop at B3BD. Upper installation uses the frozen action installer, preserving equality/negative-lock behavior and complete descriptor bank words. AF1D pose/attachment and AF30 commit remain unexecuted.

The field named `options_07f6` is misleading: the probe correctly supplies raw `$07F6`, the PRNG state, not a Setup Options value. The new verifier-repair documentation records this naming warning. A future explicit API revision should rename/document it before wiring. The frozen ABI and numeric behavior were not changed for this audit.

## Fresh replay and independent boundary guards

Fresh private MSVC `/W4 /WX` compilation of aligned/action modules and probe against immutable controller objects produced `compiled/human_pass_aligned_probe.exe`, SHA256 `86dee6d2509f80cfa398754e370da17bcdc1a7508989f2554fae0c1579afb56b`. All 129,935 compared native values pass: 94,815 left and 35,120 right. These include 20 AD0E gates, 17 choice/install/upper calls each, and three F473 calls. All three natural lane calls report obstruction; there is no natural clear-lane or endpoint/extreme-coordinate witness. Three gates stop at AD3D; seventeen reach AF1D. No B3BD or AF30 branch is naturally witnessed.

The left/right manifests remain `e328d6908108e8b4fcfb315de15bc3e54270f3a3664709056e2f1e3e95528112` and `4ddc9fffa7d0f2b756a23ef3478acc3f4306eec249ae1894a878ba9fee803477`. The raw left court-270 call has passer `$3DEB`, receiver `$3BEB`, distance 110, relative 0, fine 4 and movement direction 2. F473 returns AA=1 at event 26; event 27 at AED9 has request `$2A`, family 0, retained through AF1D event 30. The older partial adapter's `$2F`/family1 was a port implementation gap, not an original bug.

Independent `human_pass_aligned_audit_probe.c` and `test_human_pass_aligned_source_audit.py` pass 731 literal guards. They cover 576 two-SBC combinations using a separate carry/ones-complement arithmetic oracle, ten nonzero-relative cases, 98 ordinary half-open endpoint points, twelve wrapped coordinates, six list-order/skip/early-stop cases, two invalid cursors, eight installer continuation/family-order cases, eight wrapped early gates and eleven profile/distance/PRNG choices. These are controlled C source-contract tests, not new native captures. The report is `source-contract.json`.

The probe consumes actual entry raw paths, original ROM profile bytes and immutable asset descriptors; no expected exit value enters C. It checks all declared persistent vectors (including unchanged ball/controller/context/profile/order/selection records). The standalone lane mode checks AA plus seven source-saved DP words, not every volatile scratch register or CPU state. Earlier selection/initializer/action events establish ordering but are explicitly not replayed here. AD3D's catch body and `[00]+42` access, B3BD, pose/attachment, commit, airborne branches and caller return remain outside acceptance. No production manifest or enable was added.

## Original protocol rejection

Original verifier SHA256 `6da5958ab01843047c3b4c5f1fc7d0b5abc1e7af352aa0d533571ee32d899d80` passes its sixty local cases. Independent `test_human_aligned_protocol_audit.py` rejects seven malformed schema/value/vector cases but accepts all three invalid diagnostic streams: added error text, missing asset-loader line and a forged different-pack line. Lines 373–375 record stderr but do not validate it. The original report remains `independent-protocol-v2/report.json`; the earlier generic harness failure selecting a lane row without `dp_words` remains in `independent-protocol` and is not counted as a verifier defect. The adapted tool only selects a row with the applicable schema.

The repair must validate the exact asset-loader diagnostic and actual integer zero exit status in a separate verifier revision. It must not alter C, capture bytes or expected native results.
