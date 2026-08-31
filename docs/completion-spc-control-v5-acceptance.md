# SPC F1 control verifier v5 acceptance

**PASS for the bounded F1 commit component with the repaired native callback gate.** Original C, native snapshots, earlier freezes and the v4 rejection are unchanged. This supplements `completion-spc-init-control-v4-independent-audit.md`, whose initializer acceptance and control-v4 rejection remain intact.

The scheduler freeze `.analysis/spc-control-freeze-v5.json`, SHA-256 `4c9218e7642c4bb44061fd43de73b6fbb2ba23746d6585e5fe87cc37efe42cb3`, was independently rehashed: all 222 identities match. The auditor copied the exact frozen source/dependency closure and freshly compiled the two C files with `/W4 /WX`, without warnings or borrowed build objects. Results are retained under `build/spc-control-audit-v5`.

The new wrapper first applies every accepted v4 state/schema/address/value/PC/write-enable check, then requires SPC PS.P ($20) clear. Pinned Mesen source and original resident opcodes support exactly this condition: `8F 30 F1` at $0384 and `8F 01 F1` at $03EC resolve the destination through GetDirectAddress; P=1 would write $01F1 rather than the captured $00F1. MOV_Imm preserves PS. The before and after callback states are both checked. This is an SPC address bit, not the 65816 decimal flag.

Independent results:

- Both original same-clock publications pass 70 visible fields and two full ARAM endpoints with the freshly compiled probe.
- The unchanged independent two-case direct-page tool now rejects both original source-incompatible metadata mutations. All eight earlier independent boundary corruptions also reject.
- All 32 original C/regression cases, 11 evidence-integrity cases and four process-protocol cases pass.
- All 512 source-domain cases pass: 256 PS values for each callback. Only P is newly restricted; other PS bits retain their previous validity.
- The four binary inputs/outputs are byte-for-byte identical to the auditor's v4 baseline. Exact hashes are retained in `preservation.json`; no fixture or C state was changed to satisfy the proof.

The F1 hardware API has no PS argument and is unchanged. Write-disabled commits remain covered and valid at that API boundary; native callback evidence separately requires write-enable because the callback occurs inside the enabled-write branch. Directional CPU-input/SPC-output latches, underlying ARAM effects, timer enable-edge behavior and hidden staged-input semantics remain as reviewed in the original source audit.

| Object | SHA-256 |
| --- | --- |
| v5 verifier | `5579e02de639a435615b08e60ebdc1050c444d7ffa579addf3f9fea04a6ce4f0` |
| v5 callback wrapper | `2873b94e80d28f66471a34e21459a23972c526792b36814b20513a346d3b7414` |
| Unchanged control C | `3fb6f35612eb0079ceabeceabefa24af73c0af809ff35f6095becec041cde101` |
| Fresh private executable | `8f5653b8659f0e6e5cf61359963b70f142cf4c1f373a90286a88a709f0d66a5a` |
| Unchanged independent P-bit tool | `a120563cbed1612bcdf8e07373cef869eb127ed075914a5df1ed27a7890db22a` |

Acceptance is limited to the isolated post-cycle F1 commit and the observed visible fields/endpoints. Hidden staged inputs and their pending flag are synthetic source-contract tests, not Lua-observed native initial values. No clock advancement, normal SPC startup, timer/DSP continuation, audio acknowledgement schedule, cross-clock adapter, whole scheduler phase or production integration is accepted here.
