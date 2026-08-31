# Remaining CPU image-anchor migration

This checkpoint updates the C-only regression anchors at frames3480,6932 and
6954 after independent attribution. It is not native frame-parity evidence or
full-game acceptance.

An immutable pre-HUD control reproduced all three old expected hashes. A fresh,
detached clean checkout of accepted HUD commit `9c69275`, built from the current
40-source manifest with `/W4 /WX`, reproduced the current3480 and6932 images and
the pre-graphics6954 image. The accepted later HUD is pixel-identical at those
three anchors, so the C1/contact and culling checkpoints contribute no pixels.
The accepted direction/body correction leaves3480 and6932 identical and changes
only6954:957 pixels in player-pose/appearance bounds x88..229, y76..138.

The independently recommended C expectations are:

| Frame | RGB SHA-256 |
|---:|---|
|3480 | `805bc5705a4faa6946bf484fe670614767a0f34a2465e490343c5cbff79e6662` |
|6932 | `c9e82ab29223a7b5953fc795a48ce010c1a6314819f189216cc36f2f6fbf584e` |
|6954 | `9cad6d013aca98e32b28e4cb792ddcfc95b3ea7d61621d3b92b4b9857c6553cf` |

The independent report
`checkpoint-qa-20260831/build/remaining-golden-independent-audit.md` has SHA-256
`a46af369e67ab83b5c02981823932be2982238ac33c923a2c805a9e6dfbcba6d`.
Its machine report begins `6e581250`; its runner begins `d6716a8e`. The reviewed
pre-edit test identity was
`f0f8ac8a`. This migration changes only the three expected strings and their
provenance comment.

Root then built a fresh40-source `/W4 /WX` CLI and ran the complete63,800-frame
test without `--reuse-trace`. The executable is
`a91bb6702a89f1f2f6aa629c84a07a2393e3f49c961ab8dfdb62a28eca466452`;
its build manifest is
`e2fe3796b65c7016f91849fb76332b79c1d9ae49e3c87f786ff80cbf608183e8`.
The test source is
`61608b4e35fc9352004ab0f91519f3bf49d32da77b765e4205c076452b7bc0d4`,
the unchanged264-resource pack is
`f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`,
and the user ROM is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The run passed all image, gameplay/state, pass/release, shot-state, contact,
motion and action-integration checks. This closes the old C regression stop;
it does not claim a native frame match or any still-open full-game route.
