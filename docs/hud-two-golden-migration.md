# Two C-only HUD image anchors

This separate test migration updates only frames600 and1300 after independent
visual and pixel review of the accepted HUD lifecycle correction. No production
code changes. Other image anchors and every behavioral assertion remain intact.
The old C1 freeze and original image artifacts remain unchanged.

| Frame | Previous C RGB hash | Accepted HUD C RGB hash |
|---|---|---|
|600|9667d6ab5e12b2288d1b86322c13a1a5352dc31ba99ac31d72842da1b3a71264|b555afbcbfeb868b1a56b255ea15259214d9d9c49044d294b34c7d34c400853f|
|1300|7b639465715269c8b25569da4eca00d8a3f7377d018700923ebcdab1c4ed6831|4a778fc91a809ea99e723b0054d21f499ca64beef1fb2964781ed779f8d70a01|

Old BMPs reproduce both previous expected hashes. Current facd818 renders and
the accepted pre-C1 HUD executable give identical images with the same pack.
Only the old static scoreboard rectangle x32..239,y144..207 changes:5574pixels
at600 and5593at1300. Every outside pixel remains identical. The original old
panel reads WEST2/ORLANDO0/11:49; the accepted HUD view shows the underlying
court/players there. This migration follows that reviewed lifecycle repair;
it is not a claim of native pixel or frame parity.

Independent receipt `checkpoint-qa-20260831/build/hud-two-image-golden-review.md`
SHA256239ab814dcc8bd4a58e4745f33d5ffbd887e81334916bae5f91e1d59c26d07e8.
Comparison artifacts and all old/current/accepted-HUD images are retained in
primary `.analysis/progress-screenshots/20260831T190856.946784Z-c1-supplement`.
The unchanged prior image freeze is3bc921054e89d4af5a0c177a045b1ce44b57ee9fac44a7ea5f1bddaf4dbcfccb.

Full regression rerun and exact test/executable/trace/log hashes are recorded
under ignored `build/hud-two-golden-migration-v1`. The complete rerun exits1:
gameplay/state/pass/receiver/motion and both migrated images pass, then frame3480
fails with actual RGB805bc5705a4faa6946bf484fe670614767a0f34a2465e490343c5cbff79e6662.
Later image and source-marker checks do not run. The3480/6932/6954 anchors remain
unchanged and require separate attribution before any migration. No full-suite
pass is inferred.

Executed test SHA256f0f8ac8a8080a8d89c2d5f6575fcd73ed88686484c38105c897276c73ff7e681;
CLI SHA2562cb7a6713820173cca43438169195bb901bbd24832911cb58bb1012cfe3d4068.
The retained63,800-frame C1 trace remains
f69d2637d2bf2da6c9fcebfc90f70bbb2f47acc869aa70df9f38115f715e8236.
