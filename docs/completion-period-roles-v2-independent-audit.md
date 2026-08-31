# Period role continuation v2: bounded source acceptance, verifier rejection

The two frozen C modules pass independent source and native-boundary review within the documented period domain. The original v2 verifier is **not accepted** because it admits out-of-range byte fields in its first output boundary. This is a verifier-only schema defect; no C repair or native fixture change is indicated.

The scheduler freeze `.analysis/period-roles-freeze-v2.json`, SHA-256 `c8ba8e82c18518f788871656a4410a3f75ffbe3ac2394e7680b6adedcb19b2c6`, was independently rehashed: 1,013 identities, including the unchanged v1 packet. Exact source/dependency snapshots and all results are retained in auditor `build/period-roles-audit-v2`.

## Source and callers

Original `$86:E1E5–E1F6` calls `$85:BC07` with context `$46EB`, then `$476B`. The C continuation preserves this order. Current/base/alternate assignments must each be reciprocal cross-context bijections; actor IDs, groups and context pointers must be canonical; each context's five order bytes names the opposing five actors. Live state must be `$81` or `$82`, ball pointer `$0910` must be `$3EEB`, and native inputs require binary 16-bit arithmetic and DP0. These are explicit bounded preconditions, not assumptions of general BC07 correctness.

The reviewed source behavior includes:

- BC07's initial scan uses physical ball XY for a nonnegative owner and predicted `$0918/$091A` otherwise. Assignment geometry uses the actual `$879C7B` table and `$85:F34F` arithmetic. Wrapped subtraction signs, `$8000` negation, ASL truncation and the `(0,1)` direction 1/distance 0 quirk are preserved and commented. Direction 8 leaves both old facing words unchanged.
- The focal scan uses wrapped absolute values and `CMP`/N. A strict lower comparison publishes the nearest pointer; a no-winner scan retains its previous scratch pointer. Cadence consumes the actual full `$C6`, carries negative/overflow behavior, and does not clear rebuild state on an early return.
- `$BD19` first copies alternate assignments for the entry context. `$BD55` and `$BDB2` then copy base assignments in opposing-context, entry-context order. These passes cannot be collapsed: each eligible `$B95C` call consumes the shared RNG. `$BE03` clears rebuild only after all passes.
- `$B95E` clears actor `$7E` before the live `$82`/inbound-ID early return. That return preserves reaction `$60` and RNG. Other calls use physical ball `$3EEF/$3EF3`; `$B9C0` clamps through wrapped CMP/N. `$80:CEE7–CEFC` advances `$07F6` by ASL/conditional XOR `$1D87`, with zero recovery `$9146`. This original behavior is retained without changing a global RNG or velocity helper.
- The planner measures opposing actors against the entry anchor and publishes `$09E2`. Live `$81/$82` bypasses the general assignment-cleanup paths. `$BF89` selects through the ball record's `$74`, not an inferred owner or defending side. `$BFC7` is unsigned BCS, unlike adjacent wrapped-sign comparisons.
- In the ownerless/negative-receiver branch, `$C05F–C064` retains its exact CMP #6/BMI/BEQ decision. The fallback at `$C086` promotes the last equal eligible candidate. Its no-candidate return remains intact. `$C0B4–C0F5` scans the five literal order bytes and consumes `$09DA` down to zero.

The source stops before unrepresented record reads at `$BF51`, `$BFAB` and `$C05C`, and before assignment child calls at `$BF5B`, `$BF98` and `$C0DD`. No synthetic child return enters C. The naturally captured `$09DE=$00A5` is not normalized into an actor pointer: a controlled rebuild reaches the explicit `$BF51` stop when the original would read that alias. The four native witnesses return before this point, so natural planner or alias-read parity is not established.

The ordinary API checks its structural domain at begin. Repeated advance at a waiting boundary does not mutate work/state; only FIRST_RETURN may resume. The fresh probe checks these conditions on every accepted run. No production source list, gameplay enabling, timing or raw fixture was changed.

## Independent verification

After rehashing all identities, the first private compile revealed that the auditor's generic snapshot helper had omitted two v1 dependencies named under dependency keys. That failed private attempt is retained. Copying the already hashed dependencies by their actual source paths fixed the snapshot; `compiled-v2` freshly compiles all three C files with `/W4 /WX`, without borrowed root objects or compiler warnings.

- Four unchanged captures match all 223 typed fields at the final `$E1F7` boundary, or 892 field comparisons. Their added fields are preserved on the native early-return route. Native FIRST_RETURN has metadata/schema checks only: there is no captured intermediate state to compare its 223 values against.
- The original 116 source cases, 13 C contracts and 17 reachable protocol corruptions pass against the private build. The bounded reference executes original opcodes/operands, uses source CMP/C/N semantics and enforces an explicit owned-byte read projection. The single C0C2 high byte is ignored by original AND #$00FF. Host diagnostic call stacks do not claim real stack/register fidelity.
- Independently selected 640 additional valid-domain cases exercise full-word and edge values, distinct randomized reciprocal assignment sets, order permutations, actor modes, reaction/boost fields, coordinates, camera/cadence/rebuild, owner/receiver, RNG and explicit unresolved stops. They match 270,499 typed boundary fields over 1,170,796 original-ROM instruction decisions and 507 visited PCs. Both intermediate and final values are compared to the reviewed source diagnostic in these controlled tests. The actual original bytes at reaction, rebuild, primary/ownerless planner and RNG blocks were separately decoded and inspected in `raw-rom-selected.txt`.

The extra input-selection tool imports the frozen, reviewed ROM diagnostic; it is not an independently implemented second CPU diagnostic. Its independence is in input construction, domain coverage and fresh C execution. Neither that tool nor the existing reference establishes normal reachability, cycle accuracy, CPU register/stack residue, or unrepresented helper behavior.

## Concrete original verifier rejection

`verify_period_roles_v2.run_probe` validates every output value against 0..65535, ignoring `mapping()`'s byte widths. The first boundary's `contexts[0].order_49[0]` maps to byte `$4634`. Replacing it with 256 or 65535 passes the entire native comparison because only final values are compared with the native endpoint. Both are impossible in that declared byte projection, irrespective of the absence of a native intermediate-state witness.

The independent `tools/test_period_roles_protocol_audit.py` exercises the real native-case verifier with unchanged raw/C files. Its baseline passes; both first-byte corruptions incorrectly pass; final-byte overflow and nine other row/word/order controls reject. The final-byte case is rejected only by endpoint comparison, not by the missing byte-domain gate. The report is `independent-protocol-v1/report.json`.

Required repair: a new verifier must apply each mapped width's numeric domain to every output row. It must not invent native intermediate values or modify C/evidence. The original failure and frozen verifier are retained for comparison.

## Identities and limits

| Object | SHA-256 |
| --- | --- |
| Original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| v1 prefix C | `3fdf32f11510354ab832985326a9398b9a79f82a81422a60fcc5781948268072` |
| v2 continuation C | `6c7e0e02188fb1825c2581edd2ef5c69fe3ef4aa9a655800218981d8cdbef6ea` |
| v2 header | `1784f981a01f18558c1f52490bca1cabb6f3727359152f184075e01e677bd6e3` |
| Original v2 verifier | `ef7fb06ad6f8d49eacfec2ccb2cfce11b4ac0e85043479cafe5532e977fbfb4c` |
| Reviewed ROM diagnostic | `965c2a188ddcf3ee0bba53299eaedb13899e02c3c74a99bf44c9eeb91bd6f0f6` |
| Fresh private executable | `5c9d6cb1d799e23aa98c841800c079c944c51f916364bf8654762a235e60d7fe` |
| Diverse input audit | `8b718b6f2603a9f0c1a8bf7b90437dcbba53fe8e0551c593317b957e2cfbc9ce` |
| Protocol audit | `2dfac685d98d119d840f4b47eb3e607baf1f5f164cde155c8e78194008715f19` |

The source intervals have hashes in `native-v1/report.json`; their inclusion does not imply execution of excluded child branches. This audit does not accept general live-play BC07, arbitrary aliases, normal planner reachability, human play, audio, a scheduler phase model, the render tail, or a combined whole-period execution. The latter components retain separate acceptance gates.
