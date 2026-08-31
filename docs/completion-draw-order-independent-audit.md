# Independent draw-order component review

Accepted for its bounded typed order/depth contract. This does not accept
runtime scheduling, OAM publication, whole gameplay, or a replacement for
the separately accepted exceptional `80:FBFF` full sort.

The reviewed scheduler freeze is `.analysis/draw-order-freeze-v1.json`, SHA256
`3a4aea26e7e9d4e911a67326053a98f0c8d4e905bfd08be9738ec3535e7053f0`.
All301 recorded identities were independently rehashed before and after the
review without mismatch. Source and tools were copied byte-for-byte into
the auditor's `build/draw-order-audit-v1/private`; a fresh two-source
MSVC `/W4 /WX /O2 /MD` build borrowed no existing objects.

| Checked behavior | Independent conclusion |
|---|---|
| `80:FBE9..FBFE`, caller `86:DA89` | Writes exactly12 ascending original record pointers, preserves carried depths. Arbitrary previous pointer contents are overwritten. |
| `87:A3B6..A3CE` | Visits the carried order backwards, wraps Y-X at16 bits, performs two sign-preserving CMP/ROR shifts, then wraps camera-Y subtraction. No Z/fraction/screen-coordinate substitution. |
| `80:FC80..FCA1` | Exactly11 reverse adjacent comparisons. Tests the N flag of wrapped right-minus-left; equal keys remain stable. One pass is not a complete sort. |
| Input limits | Project/pass/update require a bijection of the12 original pointers and refuse invalid/null inputs without changing owned state. This is an explicit typed API limit, not an alteration of native behavior. |
| Basket provenance | `86:DBC2` is the original zero-store to basket Y. Both captured endpoints happen to be zero; the observed equality alone cannot prove the store. Basket XY remain explicit caller-owned inputs. |

The literal byte ranges were checked against the original ROM SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Inspection of the frozen test-only instruction executor confirmed its
16-bit flags, indexed addressing, CMP/ROR carry propagation, and branch
semantics for these isolated blocks. It does not execute the excluded
screen/culling/indicator body. Source comments retain wrapped comparisons,
negative rounding, ties and partial sorting rather than correcting them
into host-language sorting behavior.

Fresh native comparison passes66 immutable observed boundaries,
37 component cases/888 order-depth words, and a further12-step chain/288
words carrying C's previous order/depth. Later chain inputs supply changing
native XY/camera only. Capture code uses cold boot and ordinary menu/input
selection; no WRAM/register/ROM seeds were found. The gate validates exact
route/artifact/source/settings sets, typed duplicate-free rows, chronological
callback order, binary16/DP0/DBR7E and bounded call stack relationships.
The first projection prestate follows unmodeled gameplay since initialization;
the chain is not an initialization-to-game prediction.

Fresh supplied tests pass5,668 original-ROM cases/136,032 words,
24 refusal/initialization cases, seven null/preservation checks per probe,
11 persistent sorting passes, six invalid binary records, and46 reachable
protocol checks. Their reports are retained under `source-v1` and
`protocol-v1`; the complete natural differential is under `native-v1`.

The additional auditor tool `tools/test_draw_order_independent_cases.py`
chooses7,513 new cases/180,312 compared words. It executes every one of65,536
possible wrapped Y-X values across varied record orders and camera values,
plus2,048 carried source cases and literal tie/wrap witnesses. A separate
signed floor-division calculation also checks the CMP/ROR projection.
It passes1,935,825 original instruction decisions. This tool deliberately
reuses the reviewed frozen ROM executor; it is independent case selection
and an arithmetic cross-check, not a second independent CPU implementation.
It claims no new natural extreme-value reachability or timing evidence.

Evidence is in the auditor's `build/draw-order-audit-v1`, including initial
and final identity reports, private sources, build manifest, executable,
inputs, outputs and comparison reports. Fresh executable SHA256:
`7433d5580ec98e06e8acc7266759be82ca115f6ce474d651761d81b14058af58`.
Reviewed C source SHA256:
`90e75c6257cadeebdf923604d43d1b54e294124e533901a91ab654799972dfda`;
header `3f37a6bccf9ff22e2d945bcb400de2626ed35ba456ee48523b14cb0f6bdf3093`;
verifier `ba7c000d73a47ddeb27e7ad282d7eb8baf343bc9ae06b01109a6522c405092af`.
No candidate or frozen source, capture, existing render helper, or production
manifest was changed. Any later runtime integration must use one persistent
order/depth owner and import actual exceptional-sort results into it.
