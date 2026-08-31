# Wired culling correction checkpoint

The original depth-minus-height branch is now corrected in the existing
production helper. Independent QA accepted the bounded candidate; root copied
all five deliverables exactly and rebuilt the probe and entire40-source CLI.
This does not claim completion of the ordinary draw pass, OAM or native timing.

Candidate freeze72a30db1b79c82e99c966e8460d0edb443fb132a826c15fbbf72f59b6c976bfe
binds363files; root rehashed every identity. Independent receipt is
`checkpoint-qa-20260831/build/d1-culling-independent-audit.md`, SHA256
c3c06ee402709387f940643b8dbe12cddc34272b433dd01efc09aee5902dc438.
The unchanged candidate report `court-culling-correction.md` records its original
pre-review state. This document records subsequent acceptance and integration.

Fresh root checks under `build/culling-integration-v1` pass983,040 controlled
original-instruction cases and three malformed-input cases. Independent QA
also passes111,664 mixed cases. The fresh unchanged baseline fails196,508 of
the source cases. These are controlled Boolean tests, not natural extreme-value
reachability or complete CPU/OAM state comparisons.

Root's fresh63,800-frame trace exactly reproduces the reviewed candidate:
e1e7932d12cf29afac79a33c77f087ec1f417b172815c1f1a7aaeb3519a305f5.
Fresh CLI SHA2566f6fa3a61ed8145f6ae003dedbdb28d4c7e7e1445e734f0948a7899c4dc5eea1.
The real contact guard still finds one interruption and recovery, no unfinished
episode. The independent full trace comparison finds only22 actor6 visibility
changes versus C1, first at54152: projected depth-40, height0, CPU control.
All other serialized fields remain identical. The old port hid that actor;
the original culling branch retains it. On-screen pixels still depend on sprite
clipping; a visibility flag alone does not promise a visible sprite.

The existing panorama fixture exits20 with both unchanged and corrected source
and the same HUD pack. It is a retained pre-existing failure, not a passing
test; four later period scenarios do not run. The complete CPU regression
also still has the separately recorded3480 image difference after migration
of only the two accepted600/1300 anchors. No additional expected image changed.

Exact-commit screenshot publication follows this commit. Main and the desktop
executable remain untouched. Next D1 work covers source draw direction, ordered
actor/OAM processing, shared queue/DP ownership and exceptional period callers.
