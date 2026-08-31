# Natural human mode15 pass continuation

This separate bounded slice translates the actual mode15 dispatch, its
normal animation gate, family5 facing child, waiting ball attachment,
post-launch timer suffix and later team-mode normalization. It compares
517 naturally observed updates from30 human passes, totaling2,878,464 C
values. It stops before86:99C4 launch. That child is observed natively,
not executed by this C module. Human play remains gated.

Eleven new source/probe/capture/doc files and private output under
`build/human-pass-release` leave all fourteen earlier freezes untouched.
There are no source-manifest changes, production callers, root writes,
commits, new human enablement, or calls to the historical whole initializer.
The probe freshly compiles this module, the already accepted pose module,
asset loader and its rendering/validation dependencies under `/W4 /WX`.
It uses no old gameplay objects, session ABI or CPU/opcode interpreter.

## Source contracts

| Original source | New bounded contract |
| --- | --- |
| 87:9244-9258 | Read mode15's actual table words into DP8E/90. |
| 87:9BD0; 9C53-9C57 | Native indirect trampoline reaches wrapper879C53, which JSLs86A6B3 then RTLs. This is two return frames, not a direct table entry toA6B3. |
| 86:A6B3-A6CA | Nonowner decrements actor+60 by actual C6 with16-bit wrapping. Nonnegative timer returns. |
| 86:A772-A776; 9846-986C | Negative timer normalizes mode from actor+6E versus093A, sets+64=47, clears+60/+7E/+28. |
| 86:A6CB-A747 | Owner uses source CMP/sign decisions and upper2A..31/phase3A thresholds. |
| 86:A7A8-A7D9 | Family5 child turns facing one step toward direction66. |
| 86:A790-A79F | Reuse provenB649/B832 on existing pose resources, then add actorZ to scratch04. No resolver call on this path. |
| 86:A749-A75B | Negative receiver uses shared9861 cleanup then mode11 and threeFFFF globals; otherwise clear09C4, setAA, read actual879C7B receiver pointer and stop before99C4. |
| 86:A75F-A76F | Separate component after an actual99C4 return: assign timer10 only if wrapped CMP(family,4) has N set. |

The API returns explicit original boundaries. Family2 stops beforeA629;
family3 writes family2 and stops beforeA6F8/AEC3; unassigned non-inbound
actors stop before85:AD6B after any preceding family turn. Those children
are not silently approximated. Receiver pointer input is the original
ten-word879C7B table, not an actor chosen by possession or host slot.
Positive receiver indices outside0..9 fail transactionally rather than
inventing native out-of-table reads. The existing pose resource domain
checks remain intact. The caller is responsible for the real stack frames.

Original quirks are retained in source comments. A6D0/A70C/A764 test the
sign of a wrapped subtraction, not signed C operand ordering. A7B5 checks
the raw facing-minus-direction difference before AND7: a nonzero multiple
of8 still turns after masking to0. Eighteen hand-derived source guards
exercise these edges, cancellation, normalization and explicit missing
children. They use modified input copies only and add no native witnesses.

## Fresh evidence

Original ROM,1,572,864 bytes:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen executable:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Asset pack,89,438,786 bytes/263 assets:
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
The probe verifies49,536 animation/attachment asset bytes against the
original ROM before comparing any component. It reads the release
thresholds and actor pointer table directly from that same ROM.

Fresh `reference-v4` binds Ghidra and bounded recompiler source, original
routine/table bytes, tool hashes, commands and logs. It includes9846,
A6B3, A7A8, actual879C53 wrapper and the85:EEEE IRQ child needed for the
observed volatile-state attribution below. The99C4 launch source is
included for identifying the next missing child; no launch C proof is
claimed by having that source available.

Final captures run sequentially in fresh private Mesen processes with
private executable/settings/home/saves and fixed zero power-on RAM.
Both follow ordinary front-end input into2,400 court frames. Fresh
controller selections are `[2,1,1,1,1]`; left uses two released left taps,
right retains2. Court input cycles the nine direction choices, with a
three-frame B pulse and a32-frame B hold in each120-frame block. No
ROM/WRAM/PC write or controller-owner reassignment occurs.

The script retains the actual84:E2AC prestate only when the original B
edge leads toDF7A. It observes initializerAF4D and human return8791C3,
then follows that actual actor's mode15 dispatches. It captures A/X/Y/PS,
D/SP/DBR/program-bank/PC and13,824 raw bytes at0000-1FFF and3400-49FF.
The bus-read stack preview is limited to23 bytes and stops at1FFF.

| Final capture | Events | Human passes | Mode calls | Wait / launch / timer / normalize | C values |
| --- | ---: | ---: | ---: | --- | ---: |
| left selection0-v3 | 1,886 | 13 | 185 | 81 /13 /78 /13 | 1,071,928 |
| right selection2-v3 | 3,162 | 17 | 332 | 102 /17 /197 /16 | 1,806,536 |
| total | 5,048 | 30 | 517 | 183 /30 /275 /29 | 2,878,464 |

Every declared output is compared: result,128 DP words,1,408 actor/ball
words,160 controller words,128 context words,20 profile words,13 order
words and16 globals,1,874 values per component. Separate dispatch, step,
turn, offset, attachment, normalization and post-launch suffix components
use only their own actual entry snapshot. The post-launch suffix input is
its distinctA75F native entry; it is never used to seed or pretend to close
the missing99C4 body in a combined replay.

Six left and eleven right native99C4 calls occur with B still held; seven
left and six right occur after B is up. Mode15 release is animation-gated
in these observed routes, not conditioned on a physical B-up edge. Families
FFFF/0/1/5 are observed on the left andFFFF/5 on the right. No family2/3/4,
cancellation, unassigned steering or point1 attachment is naturally covered.

The final right pass launches at court2373. At the fixed capture end,
its last observed mode call at2399 still has timer19, mode15 and no ball
ownership. The verifier records that explicit pending cleanup. It does
not claim that this thirtieth normalization was observed; the29 actual
normalizations are separately compared.

## CPU, timing and interrupt scope

All runtime boundary rows require exact integer types,16-bit registers,
D=0, M/X=16-bit and DBR7E. Binary arithmetic requires decimal mode clear;
PS&38 must be0. CPU register transfer/preservation and both native wrapper
JSL/RTL frames are checked separately from the typed C memory result.
The C module does not emulate registers, status flags, hardware stack
addresses or interrupt execution.

Final left934->935 and1001->1002 cross a frame only inside the unexecuted
99C4 child. Each compared C segment remains in one frame. The verifier
allows that explicit child observation to cross at most one frame, while
retaining exact front-end and relative court clock anchors everywhere.

Native interrupts can replace discarded stack bytes belowSP during the
straight-line dispatch; active caller frames remain exact. At right1461,
05C8 additionally changesEEEE->EF14 with05CA remaining85. Original85:EF05-
EF0E installs exactly that successor IRQ pointer while preserving A/P.
The writer PC was not hooked: the report labels this as source attribution
of an observed interrupt effect. Only that exact pointer transition is
admitted outside the C projection; unknown predecessors/successors/banks
are rejected. No arbitrary volatile-memory exclusion is used.

The strict verifier binds all native source/artifact identities, executed
ROM/Mesen/script/arguments/environment, exact private settings/home/saves
before calling the mutating helper on a copy, all raw row metadata and
stack bytes, route/call/origin order, clocks and complete C response shape.
The asset loader must emit exactly its one success line naming the actual
argument path, pack size and asset count. Extra, missing or forged stderr,
noninteger process status and numeric JSON coercions are rejected.

## Preserved failures and remaining work

Build-v1/v2 retain link errors while isolating asset-loader dependencies;
build-v3 compiles and links the complete required private dependency set
without warnings. No gameplay source mismatch required a C repair.
Reference-v1 lacked helper/wrapper seeds, v2 used an oversized special
range, and v3 preceded IRQ attribution; all are retained. Capture-v1 lacked
both wrapper and point0 return hooks; v2 completed those hooks with a
12-frame hold. Finalv3 uses32 held frames. Their snapshots remain separate.

The first verifier rejected a frame crossing inside99C4; a second rejected
discarded interrupt stack bytes; the final right route additionally exposed
the05C8 IRQ target update and a still-running cleanup timer at the fixed
capture end. Failed reports and corresponding verifier snapshots are
retained. Earlier mutation-v1 was exploratory while the verifier changed;
its baseline/source identity does not attest the final verifier. Final
mutation-v2 reruns against the unchanged final source and probe.

Next missing natural boundary is86:99C4, before it saves scratch words,
detaches the ball, selects the launch table, predicts/clamps the receiver
intercept and calculates velocities. Its actual ball velocity words are
3EF9/3EFB/3EFD. The historical vector probe's shifted offsets and old
whole-initializer shortcut must not be reused as proof. The30 saved native
launch entry/exit pairs and `native-release-attribution.json` identify that
next slice; they are observations, not a completed C launch.
