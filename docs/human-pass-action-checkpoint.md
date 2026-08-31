# Side/back pass gate and grounded animation child

This checkpoint begins at the previous initializer's first `$86:AC50` and
closes the complete `$86:B00B-B04B` grounded-special child. It stops at the
actual `$86:AF1D` pose-call boundary. Other outcomes return explicit,
unexecuted continuations at AD0E, ACA9 or AFC4. No normal human dispatch,
source-manifest entry, owner transfer, root edit, commit or push is added.
All six earlier checkpoints remain unchanged.

The new `nba_human_pass_action_select` follows AC50's relative-direction gate,
the actor Z/vertical-velocity test, profile byte3E and lower-animation tests,
then the distance/boost/stationary tests in their original order. On the
grounded-special path it calls `nba_human_pass_action_grounded`. That child
sets receiver timer `$50`, zeros passer planar velocity and magnitude, writes
family5 and coarse direction into passer `+C0/+66`, sets flags6, and submits
the exact upper-animation requests. It does not set mode15, resolve resources,
move the ball or alter ownership.

The child uses the existing semantic animation command for `$87:B47A` and
adds its exact DP47/49 descriptor-word effects. The entire typed state is
projected, including unchanged lower channels, queues, profile pointers,
actor/ball records, controller records, contexts and selected globals. The
original ROM supplies the referenced profile byte; the immutable asset pack
supplies animation descriptors. The probe hashes/binds both inputs and reads
no native after-state or expected output. It redirects the asset loader's
ordinary diagnostic to stderr so stdout must consist entirely of typed JSON.

Preserved original behavior: at B039-B03E a distance of at least `$F1`
requests upper `$2C`. B042-B047 then unconditionally requests `$2F`. Both ROM
descriptors install lock `$FFFF`; B486 rejects the second request once `$2C`
is locked. Thus DP00 contains `$2F` while the upper state remains `$2C`.
This is witnessed naturally on the right route at court800 and870, upper
entries84 and110, and is documented beside the implementation. A single
assignment of the final requested pose would lose this behavior.

Fresh original captures run ordinary Title/Setup/Player Select journeys and
2400 court frames in separate private Mesen processes. Each has private
executable/settings/Lua home and initially empty saves. No ROM or emulator
state injection occurs. Native selection0 and selection2 use the same normal
B/direction input schedule as the earlier pass checkpoint.

| Compared native stage | Left | Right |
| --- | ---: | ---: |
| AC50 gate to actual continuation | 16 | 9 |
| Complete B00B child | 1 | 4 |
| B47A upper-animation child | 1 | 6 |
| Compared values | 31266 | 33003 |

All **64269 values pass**. Twenty gates stop at AD0E; five execute B00B and
stop at AF1D. Three B00B calls make one upper request; two make both requests,
including the locked rejection. Earlier DF7A, distance, cancellation and
initializer stages remain in the native trace to validate route structure,
but this probe does not replay them. Its report names those counts as observed
only. It does not establish an end-to-end combined human caller.

No normal ACA9 or boosted AFC4 outcome was witnessed, and no controlled case
is counted as native evidence. The observed side/back calls all take the
profile-below-85 gate; the other gate predicates remain source-derived. The
standalone animation observations cover requests2C/2F with nonzero descriptor
locks, not every animation state or zero-lock branch. The source reference
includes AFC4 for continuation analysis but that child is not implemented.

The strict verifier checks supported source versions, exact command and
environment, all artifact identities, private settings/home/saves before the
shared helper can mutate copied metadata, exact event word bounds, raw/event
agreement and route clock anchors. It requires each gate and its complete
child/request sequence, rejects missing stages and compares every typed C
field. The 45 rejection tests in `mutations-final-v1/report.json` are derived
from the independent pass audit test, with action fields/stages, one-frame
clock offsets and wrong immutable assets added. These are implementer-run
checks, not independent acceptance.

The preserved initializer investigation had nine output mismatches in its
partial historical adapter. `native-action-attribution.json` maps seven to
these original B00B writers: five `+66` values and the two long-pass upper
states. Each value is already established at native B00B exit and remains
unchanged through the later initializer return. The existing scene initializer
is unchanged; this is a new faithful child, not a claim that its old complete
pass path now passes. The first remaining mismatch is still left court270,
aligned native upper2A/family0 versus old C upper2F/family1.

Remaining work begins at the returned continuation. AD0E contains the catch
preinit/off-axis/aligned decision paths, including F473 and AF66/B468. ACA9
contains the normal side/back rotation/selection path. AFC4 is the boosted
child. AF1D calls the still-unexecuted pose/attachment helpers AEC3 and B649,
followed by ball Z, mode15/live-state commit and native stack restoration.
CPU registers/flags, cycles, stack bytes and unrelated volatile scratch are
not represented here. Do not substitute recorded continuation after-state or
call the old complete initializer after this child.

Files and identities are under `build/human-pass-action`:

- Original ROM SHA256
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`;
  Mesen `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
- Asset pack SHA256
  `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
- Fresh original ROM/Ghidra/recomp `reference-v1/manifest.json`, SHA256
  `2aaf2667235f28a6596de98dc7816113b3a93f25920da47c852054a482533f2f`.
  It binds AC50's gate, B00B, B47A, the upper-state table and both descriptor
  headers, plus tool-source hashes and completed commands.
- Left capture manifest
  `4aadfad8306e2b5f7e13f7352384ac6c2995a57fcd32427514a2509b36d9ff79`;
  right `736b35a1b70a598ee027a02e1cad2376d5f2b1be4cb2e49d9672740d960a2692`.
- Final probe SHA256
  `1d201f51404ddb613a6cdb8c3cf7876d357869fef2179d5e589a965fdbb3107d`.

Build with `tools/build_human_pass_action_probe.ps1`, which writes only private
outputs and links hash-bound unchanged production objects. Verify with
`tools/verify_human_pass_action.py --capture <selection*-v1> --probe
<human_pass_action_probe.exe> --rom <original ROM> --assets <immutable pack>
--output <new report>`. Run `tools/test_human_pass_action_evidence.py` with
`--verifier tools/verify_human_pass_action.py` and the same capture/probe/ROM
arguments plus a new output directory. It uses the bound default asset pack.
Earlier binaries/reports and the prior adapter failure evidence are retained.

`freeze-v1.json` binds eleven new files, dependency objects/sources, native
references/captures, reports, attribution and patch. Root owns independent
acceptance and integration before any normal human enablement.
