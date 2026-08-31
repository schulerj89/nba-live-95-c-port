# Original layout dispatch and the port's inbound stall

The port's layout1 dispatcher was wrong. At `$85:C39C`, CMP #2 sets N;
the BNE atC39F preserves it, and BPL atC3A4 tests that comparison result.
Layout1 reachesC50B, the edge initializer, just like the ordinary negative
layout. Only layout4 reachesC450's ball-X/endline path among the positive
layout values accepted here. The C code previously grouped1 with4.

The global target helper now routes1 through its existing edge branch. The
source comment names the original PCs and the previous consequence. This
corrects a port defect; it does not repair or label an original-game bug.
The earlier wrapped-direction bug preservation is unchanged.

Full suitev10 stopped its fresh Arcade/12-minute endurance journey at46450:
an inbounder at394/-219 could not reach404/-224, with300 repeatedly reloaded
atF4F2. A private old-F34F relink reproduced the same frame, inbound state and
score, isolating it from the newly accepted arithmetic correction. The retained
diagnostic trace shows layout1 was selected at44050 using ball/dead coordinates
404/-45; the wrong layout then created404/-224. Original C50B instead produces
394/-45. The old failure and diagnostic logs remain in
`build/tip-stall-attribution-v1`; full-suite failures remain unmodified.

Fresh original-ROM evidence uses real menu/controller input until the first
naturalC37D call at frame5374/court984. Case0 records that unmodified call.
Eight cases replace exactly the declared eight WRAM words at that genuine
entry, leave CPU/stack/ROM untouched, and stop at its firstC5C0. They cover
bothX signs, bothY clipping edges, opposite/same context signs, layout4
distinctions and the negative-layout control. Each has full0000..4AFF before,
entry and exit bytes, registers, observed PCs and isolated-runtime provenance.
These are controlled original-ROM source-contract witnesses, not claims that
all eight prestates occur naturally.

The accepted capture paths are `build/inbound-layout-native-v2/case0`, case1,
and `build/inbound-layout-native-v3/case2` through case8. The original all-neutral
menu attempt timed out in v1. Parallel v2case2 completed its code boundary but
failed the independent Mesen-home check; it is rejected and retained. The v3
captures ran sequentially and passed isolation. No failed artifact is reused.

`build/inbound-layout-attribution-v1/native-nine.json` passes all54 target,
direction, play/request and RNG words. Its private old-source probe fails
exactly the five layout1 cases; unchanged layout4, negative-layout and natural
controls still pass. The new verifier checks source/runner/ROM/Mesen bytes,
command/environment/settings, declared WRAM-only changes, raw sizes, CPU word
domains, routine boundaries, the captured dispatcher destination and route
clock provenance. Its33 metadata/output corruption checks all reject.
Instruction lists are branch evidence; this is not a cycle-by-cycle timing
replay or a claim to reproduce every volatile scratch/CPU return value.

After the repair, the unchanged63800-frame endurance test passes period1 and
12026 post-restart live frames. Its four controlled cancellation projections
and attached whole-update recovery binding still pass. Startup self-checks
now assert the fractional layout1 edge result as well as layout4's original
integer402 projection; the old check incorrectly used layout1 for the latter.

Full suitev11 next stopped at the C-only Mode1 frame1000 winner census.
The private pre-repair helper plus its matching old startup self-check is
relinked against the same remaining production objects. It reproduces every
old winner count750/46939/5641/1362/2652. Telemetry is identical through505;
the only changes at506 are the layout1 target/direction, selected play/RNG and
the inbounder's target fields. Renderer code and assets are unchanged.
At1000 the corrected trajectory yields0/46422/5641/1298/3983. The regression
pins those new C totals while retaining all57344 per-pixel rank, palette and
accounting checks. Native image/state expectations are unchanged. This
attribution is not native whole-frame or HUD parity.

Independent review of this repair and C-fixture attribution is pending.
Full suitev12 is in progress; no suite or whole-game completion is claimed.
The new human components remain outside the production manifest and disabled.
