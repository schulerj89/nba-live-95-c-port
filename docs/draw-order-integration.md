# Draw-order component acceptance

The typed draw-order component is accepted and copied into the integration
branch. It is **not enabled in normal gameplay**. The independent review and
root rebuild establish its stated initialization, depth projection and single
adjacent-pass operations, not renderer scheduling or a complete period repair.

The accepted scheduler freeze is `draw-order-freeze-v1.json`, SHA256
`3a4aea26e7e9d4e911a67326053a98f0c8d4e905bfd08be9738ec3535e7053f0`.
All 301 frozen identities matched before and after root verification. See the
[source contract](draw-order-source-state.md) and
[independent acceptance](completion-draw-order-independent-audit.md). The source
contract's pending-review wording records its original freeze stage; this
document records the subsequent acceptance without rewriting frozen evidence.

Root evidence is retained in `build/draw-order-integration-v1`. A fresh
two-source MSVC `/W4 /WX` build passes:

- 66 native boundaries, 37 isolated cases and 888 order/depth words;
- a 12-pass chain carrying C's own previous order/depth, with 288 more words;
- 5,668 controlled original-ROM cases, 24 refusal/initialization checks,
  11 persistent passes and six malformed binary inputs;
- 46 reachable protocol rejection cases;
- the auditor's 7,513 additional cases, covering all 65,536 wrapped Y-X
  differences and 2,048 carried source cases.

All 39 native input, output and stderr files are byte-identical to the
scheduler's accepted run. Additional controlled cases reuse the reviewed
test-only ROM executor with independent inputs and a separate arithmetic
cross-check; they are not a second CPU implementation or evidence of natural
extreme-value reachability. The initial preparation receipt accidentally
serialized its count as an array of ones; `final-identities.json` corrects
that metadata to 301 and preserves the earlier receipt.

No source-manifest, main-checkout, desktop executable, HUD, or existing period
component was changed. Runtime adoption still needs one persistent order/depth
owner, real caller-owned XY/camera inputs, original basket initialization and
imports of the exceptional `80:FBFF` full-sort results. It must not replace the
original partial pass with a host full sort or rerun initialization each frame.
