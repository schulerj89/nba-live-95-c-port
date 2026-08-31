# Human99C4 launch checkpoint

This new slice implements86:99C4 through9BB0, including its multiply/divide
contracts, target prediction, source-derived clamp children, ball release,
initial integer position step, receiver timer and conditional source cleanup.
Thirty ordinary human launches and120 arithmetic helper calls compare
exactly:283,410 values. Three separately injected native multiply cases
add5,670 controlled values. Human play remains disabled in production.

Eleven new source/probe/capture/doc files and `build/human-pass-launch`
are separate from all fifteen previous freezes. No source manifest,
existing module/header, root checkout, enable flag, old initializer or
production caller changes. No commits or pushes. The asset-free probe
freshly builds only the new module and probe with `/W4 /WX` and the
existing type header; it uses no prior gameplay objects or CPU interpreter.

## Typed source contracts

| Source | Preserved behavior |
| --- | --- |
| 86:99C4-99D6; 9B92-9BB0 | Ten PEI scratch words correspond to C locals and are restored; native physical frames are independently checked. |
| 86:99D8-9A29 | Copy actual E0/E2 words to0914/0916, detach owner, set094A=1, source+5A=20, ball pointers3EEB and durationB2 from the original family/band table. |
| 86:9A2F-9A89 | Predict receiver integer X/Y using signed multiplication followed by arithmetic shift8 and wrapped position addition. |
| 86:9A8B-9AAF; 9BB1-9C44 | Wrapped CMP signs choose court clamps. Clamp children also change the receiver's actual velocity at+E/+10. |
| 86:9AAF-9B24 | Sign-extend each wrapped16-bit target-minus-ball difference, shift8, divide by duration, write ball velocities3EF9/3EFB/3EFD. |
| 86:9B26-9B60; 9C45-9C6E | Source Z>=16 subtracts80 from vertical velocity. Arithmetic shift6 for upper2B/2C, otherwise7, supplies the initial integer position step. Ball fractions remain unchanged. |
| 86:9B63-9B91 | Receiver mode14 retains its timer; others receive duration+15. Non-mode15 source uses9846 normalization. Wrapped CMP(live,81) controls live reset. |

The state contains ten typed actors, retaining source/receiver aliasing.
The adapter resolves original96/8E pointers into those actors; it never
chooses a receiver from possession or an arbitrary host slot. Valid table
bands are0,6,12,18,24,30 in the three original36-byte tables9C6F/9C93/9CB7.
Invalid actor/table domains fail without partially changing typed state.
Positive, negative and zero family selection follows the original branch.

`F78B` exposes its actual magnitude scratch0820/0822 and085A counter.
Its temporary overlapping821 writes leave a partial cross-product high
byte at0822, not the product high word returned inX. Both operand
magnitudes are at most8000, keeping this signed path's cross sum below65536.

`F8D9` exposes CC/CE/D0, unsigned division scratch0806-0810 and sign0824.
It returns the signed quotient's low word inA, unsigned remainder low inX,
and unsigned quotient high inY. X is not a quotient high word. The source
zero-divisor branch returns quotient0 and the original magnitude remainder;
the C implementation preserves that result without undefined division.

## Original quirks retained

At85:F7D4, `SEP #$30` clears both index-register high bytes. The multiply
does not explicitly loadY, but it nevertheless truncatesY to8 bits. All30
first natural99C4 multiply calls show an actor pointer becoming00EB. Later
`REP #$30` does not restore those lost bits. The initial C assumption that
Y was preserved failed; the source module now comments and retains the
actual truncation.

At85:F7A5/F7A8, complementing magnitude lowFFFF yields0 and BEQ skips the
increment normally used for negation. Fresh controlled native execution
confirms `255 * -257` returnsFFFF0000, or-65536, rather than-65535.
`256 * -256` also returnsFFFF0000; the positive255*257 control returns
0000FFFF. The C helper deliberately preserves this original bug. These
three controlled cases are not evidence of ordinary-play reachability.

The clamp decisions and live-state comparison retain wrapped subtraction
signs. The receiver-velocity clamp writes, mode14 timer exception, unsigned
height threshold and signed-floor position steps are not normalized into
more convenient host behavior. Ball velocity addresses are the actual
3EF9/3EFB/3EFD, correcting the interpretation used by an older unrelated
vector probe without modifying that frozen probe.

## Fresh source and native evidence

Original ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`,1,572,864 bytes:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Fresh Ghidra and bounded recompiler source, exact original bytes/tables,
tool identities and commands are in `reference-v2/manifest.json`. V1's
oversized multiply range is retained; final range stops atF821 after RTL.

Natural captures run sequentially in fresh isolated Mesen processes,
using private executable/settings/home/saves and fixed zero power-on RAM.
They reproduce the ordinary front-end left0/right2 selection and court
direction/B pattern from release checkpoint15: three-frame pulses plus
32-frame holds in120-frame blocks. They never write WRAM/ROM/PC/registers.
Actual E2AC/DF7A/AF4D/8791C3 origins and both mode15 dispatch frames lead
to each actual99C4 entry. Arithmetic helper entry/exit PCs are captured
without substituting a host routine or after-state.

| Capture | Events | Launches | F78B | F8D9 | C values |
| --- | ---: | ---: | ---: | ---: | ---: |
| left selection0-v1 | 331 | 13 | 26 | 26 | 122,811 |
| right selection2-v1 | 427 | 17 | 34 | 34 | 160,599 |
| total | 758 | 30 | 60 | 60 | 283,410 |

Each launch response compares1,887 numeric values; each helper response
adds its actualA/X/Y return words for1,890. Comparisons include128 DP words,
1,408 actor/ball words,160 controller words,128 context words,20 profile
words,13 order words,19 globals,10 arithmetic globals and the result.
Every field, vector length, numeric type and response row is checked.
The asset-free process must have integer exit0 and exactly empty stderr.

The verifier binds exact ROM/Mesen/script/runner/helper identities,
executed arguments/environment, all artifacts, private settings/home/saves,
raw metadata, CPU domains, original saved frames, load operands/status,
component order and clocks. Runtime arithmetic requires D=0,16-bit M/X,
DP0 and DBR7E. The observed divide-status contract is explicitly bounded
to positive duration<=30 and coordinate dividend magnitude below2^24.
The C arithmetic supports more values; this native status proof does not
claim those unobserved inputs.

## Direct NMI attribution, not after-state seeding

The two left frame crossings occur at934->935 and1001->1002. Both now
directly capture NMI80815A through actual normal RTI80859B, including
entry/exit memory, registers, depth and the original hardware frame.
A/X/Y/D/DBR/SP are unchanged by those handler intervals. Reentrant
RTI808171 is instrumented but unwitnessed. No IRQ writer is inferred to
explain these DP changes.

The first NMI changes DP bytes39/3A/64/7C/7D/7E; the second changes
35/39/3A/6C/6E/6F/7A. Together they exactly account for the11 unowned-DP
word mismatches in the initial whole-call diagnostic. No launch-owned DP
word is exempted. The final verifier proves the observed NMI transitions
account for those specific bytes and checks that the typed C routine
preserves its original-entry values for them. Every other DP word remains
an exact comparison. C is never given an NMI or launch after-state.

The C module is a routine contract, not a replacement for native interrupt
scheduling or a generic CPU register/stack emulator. Native registers and
physical frames are checked separately. In particular, ten saved PEI
words come from the actual99C4 entry and match the9B92 stack before PLA;
9BB0's restored words, final PLA flags and actual RTL target are checked.

## Controlled and source-only limits

The three controlled runs each start in a separate fresh private Mesen
process and follow the same natural left route to the first99C4 call.
At86:9A36, only B2/B6 are deliberately overwritten before the original
LDA/LDX andF78B execution. Original values24/31 and injected values are
recorded. No ROM/PC/register patch occurs; execution stops at the first
helper's actual return boundary. Their manifests explicitly say
`state_injection: true`; they cannot pass the natural-capture verifier.
Final controlled-v2 captures and `controlled-math-report-v7.json` compare
all5,670 values, exact expected result words, preserved return frame,
private-process attestations and injection addresses. Their first eleven
rows and raw states exactly match the unchanged natural left capture.
The helper entry differs only in the declared B2/B6 words and the source
LDA/LDX result registers/flags. The three return status values follow
F7A8's skipped increment, F7AD's INX, or F81F's PLB respectively.

Fourteen additional hand-derived source guards cover zero/negative/large
division, clamp velocity writes, six-bit stepping, height adjustment and
mode14 cleanup. These use modified input copies for C only and add no
native coverage. No natural capture reaches a clamp, zero/negative divisor,
large quotient, source/receiver alias, upper2B/2C stepping, height>=16,
or non-mode15 source normalization. Those branches remain explicitly
source-derived, pending separate native evidence if integration needs them.

## Preserved failures and freeze

The old-release entry/exit diagnostic initially reported11 DP word
differences across the two frame crossings; fresh NMI boundaries explain
all of them. The first fresh helper comparison then found the realY
truncation mismatch. Its C/header/object/executable/verifier and failed
report are retained before the new-module correction.

The first integrity run accepted two forged helper-entry carry flags.
The verifier now binds those source ADC/SBC/load status contracts. A
subsequent status check initially overlooked F81F PLB settingN/Z from
restoredDBR7E on the unsigned multiply return; that failed verifier/report
is also retained, and the source-derived status check is corrected.
Final177 integrity mutations all reject. No fixture or original binary
was changed to make a comparison pass.

The first supplementary controlled captures requested a stop inside a
callback, but Mesen continued callbacks briefly after the log closed.
Their extra raw files and `capture_error.txt` are retained in controlled-v1;
the earlier permissive comparison reports are superseded and are not
successful capture evidence. A strict final file-set check exposed this.
New controlled-v2 scripts disable callbacks before closing the log and
requesting stop. Three fresh sequential processes each produced exactly
thirteen rows/raw files with no capture error. The natural verifier now
also rejects any actual unlisted file. The failed stricter controlled
checks, including an initially overbroad origin clock assumption and the
low-zero INX return-flag oversight, remain preserved.

The launch child is now translated and naturally compared for the observed
human mode15 routes. Production wiring, full human gameplay, unobserved
launch branches, and the previously explicit A629/A6F8/AD6B continuations
remain separate work. All fifteen prior freezes remain immutable.
