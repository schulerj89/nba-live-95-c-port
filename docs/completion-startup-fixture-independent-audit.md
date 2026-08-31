# Startup boosted-pass fixture independent audit

Verdict: **PASS for the isolated startup self-test repair and all-team initialization gate**. This fixes a host test precondition, not original gameplay, ratings or team selection. It does not approve the separately reviewed closure digest.

Root `build/startup-fixture-freeze-v1.json` SHA256 `f14ac4091cbc016a83a5960618c74bd2f39518e0792720fc596d2d67c01a6a39` binds 119 independently checked files. Exact private source/header copies and fresh old/new builds are under auditor `build/startup-fixture-audit-v1`.

The only `nba_tipoff.c` changes relative to the matched pre-repair 9f5b47c source are the explanatory comment, a local `NbaSession` copy, setting that copy's right/home team to Orlando18, and pointing the local synthetic state at the copy. The diff leaves all boosted-pass eligibility, mode, animation, vertical speed, apex and release assertions unchanged. It adds no successful bypass or rating override. `nba_tipoff_init` still invokes the test normally and later publishes the actual caller's team contexts. The source pointer has only local lifetime and is not returned or stored in the real game.

Independent original-ROM reads resolve the four-byte table entry at `$84:E640 + 19*4`, its 24-bit roster pointer and all twelve relative record offsets. Philadelphia's `+$3E` bytes are `67,72,70,72,77,75,75,60,72,60,62,60`, all below `$55`. The same direct extraction confirms Orlando18 has qualifying records. No asset or ROM byte was altered. `rom-profiles.json` records exact source addresses and canonical ROM SHA256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

Fresh private MSVC `/W4 /WX` compilation of both old/new tipoff and the frozen probe uses the same freshly rebuilt 9f5b47c supporting objects and matched headers. Old probe SHA256 is `efb96be818feb9457bac795931b8f349f7de68c0415b6d4e1c9e73c2e597d3bc`; new is `51cc3d13b94ae0c8d5d4882fd8bdab693820240b733aac265a1a884e05308410`.

The old build reproduces 812 successful pairs and exactly the 29 home19 failures. The new build passes all 841 pairs. For each successful call, the probe independently requires byte-identical caller session, canonical home/right context0 and visitor/left context1, and the configured clock. Quarter settings rotate across the 841 pairs; this is not all four settings for every pair.

Every 3,888-byte owned state (from offset32 through the end of the matched structure) is identical for all 812 common successful pairs. Both complete fresh state streams also match the root's frozen streams byte-for-byte. Only four leading host pointers are excluded; source inspection confirms these are assets, session and optional observer/context, not gameplay values. Comparisons are valid for this matched compiler/layout, not a portable binary serialization contract.

The new build-Test step invokes the matrix probe and fails on its nonzero result. The probe never forces initialization success: old failures remain observable and all successful state/session assertions remain active. Frozen source, old failures and native artifacts were preserved. No full-match, natural native trajectory or human-control claim follows from this startup coverage.
