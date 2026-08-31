# AD3D catch geometry, rating, RNG and lane checkpoint

This new bounded module executes `86:AD3D-AE0F`: receiver-to-basket geometry,
the actual indirect read atAD83, the profile/RNG attempt gate, `85:F02D`
direction and `85:F5E4` lane test. All three natural calls previously stopped
atAD3D now match through the already implemented AE10 choice boundary.
The clear-lane alternative executes only `86:AF66-AF82` and stops before its
unimplemented `86:B468` child. No natural AF66 witness was captured.

The eleven new files are independent of the eleven earlier freezes. Outputs,
captures, references and the patch/freeze reside in `build/human-pass-catch`
in `completion-controllers`. No existing frozen source, production manifest,
root checkout, old complete initializer, session ABI or human-enable flag is
changed. There is no commit or push.

## Source contracts

| New API | Exact boundary |
| --- | --- |
| `nba_human_pass_catch_geometry` | AD3D toAD98: wrapped geometry, AD83 actual `[00]+42`, D04A lookup and threshold. |
| `nba_human_pass_catch_rng` | 80:CEE7 through its original RTL: zero fallback9146, otherwise shifted seed and conditional XOR1D87. |
| `nba_human_pass_catch_direction` | 85:F02D through its original RTL: AA direction, AE major magnitude, B2 quadrant/index scratch. This is not F34F. |
| `nba_human_pass_catch_lane` | 85:F5E4 toF727: original asymmetric box, forward/reverse order-list scan and exact volatile outputs. |
| `nba_human_pass_catch_attempt` | AD98 toAE10 orAF66, including actual profile word at`[E0]+39`, RNG and both children when called. |
| `nba_human_pass_catch_receiver` | AF66 to actual B468 entry, before the child executes. Computes receiver timer and exchanges96/8E. |
| `nba_human_pass_catch_prepare` | Combined AD3D toAE10 or unexecuted B468. |

The module uses typed actors, slots and scratch words. No opcode interpreter,
later native state, owner reassignment or old initializer supplies results.
Input words come from the actual captured prestate bus pointers. The adapter
supports captured WRAM, low WRAM mirrors and in-ROM LoROM addresses; unknown
or uncaptured addresses fail rather than becoming zero. There are ten player
slots and the ball atslot10. The original13-word order list includes zero
sentinels atboth ends; actor order cursors remain literal1..11. Receiver timer
setup accepts the initializer's source band offsets0/6/12/18/24/30.

## Original evidence

Original ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen executable:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
The unchanged asset pack loaded by the probe:
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

Fresh Ghidra 65816 listings and recompiler output cover `80:CEE7-CEFC`,
`85:F02D-F099`, comparison-only `85:F347-F3BA`, `85:F5E4-F727`,
`86:AD3D-AE0F` and `86:AF66-AFA5`. The reference retains exact rating,
direction and band table bytes, including the source bytes atband30.
`reference-v2/manifest.json` binds commands, logs, immutable tool/source hashes
and artifacts; SHA-256:
`cc2317475cad4ad631c732b72a2336a6176f76d4aa2fa1572bd966eb93c1af2c`.
Reference-v1 remains preserved but is not normative: its F02D range included
unneeded following code and its30-byte band span omitted the band30 read.

Left and right captures ran sequentially in separate fresh private Mesen
processes, with private settings/home/saves, fixed zero RAM and ordinary
Player-screen/controller input for2,400 court frames. Left uses two released
left taps from the fresh `[2,1,1,1,1]` selections; right retains selection2.
Directed and neutral B inputs are ordinary controller input. There are no
WRAM/ROM/PC patches, controlled state injections or after-state seeds.

| Route | Events | Pass calls observed | AD3D calls compared | Compared values |
| --- | ---: | ---: | ---: | ---: |
| left0 | 644 | 16 | 3 | 31,788 |
| right2 | 361 | 9 | 0 | 0, observation only |

The left comparison passes three executions each of geometry, RNG, direction,
lane, attempt and the combined AD3D prefix. Every response has1,766 numeric
values: three input words, seventeen DP words, all1,408 actor words,160
controller words,128 context words,20 profile-table words,13 order words,
sixteen globals and one result/route. Earlier selection/initializer/action
and later pose stages are structurally observed, not replayed by this probe.

The right report explicitly states `capture_attested:true`,
`comparison_performed:false`, nine pass calls and zero catch calls. It has no
`passed` field and never invokes C. Normal comparison mode rejects zero catch
calls; observation mode cannot skip any existing catch call. The two captures
do not establish natural catch coverage for both controller sides.

Capture manifest hashes:

- `selection0-v1/manifest.json`:
  `90698c47248b6788fa3009ead4e824df5be517b8ebfa4a8760f82ed1e6382669`
- `selection2-v1/manifest.json`:
  `b23a5b41c8eae90cfb9f8336d20fad12c368379f23886ce6626049d51f203049`

## Actual pointer and child attribution

AD83 reads `[00]+42`, while ADA9 separately reads `[E0]+39`. The captured
AD83 bus address and word are independently checked against sparse WRAM or
the immutable ROM, including CPUY42 and the later table-indexX. The actual
AD83 values are WRAM mirrors; replacing them with a roster-profile field
would change the original game. That native quirk is preserved with PCs in C.

| Court | Actual AD83 bus read | Distance / threshold | Actual profile low byte | RNG before toafter | F02D direction | F5E4 AA | Final0904 |
| --- | --- | --- | ---: | --- | ---: | ---: | ---: |
| 1591 | 00004C = 7E50 | 94 /157 | 91 | 24456 to48912 | 4 | 1 | 1 |
| 1641 | 06004C = 7E50 | 59 /157 | 80 | 45081 to32181 | 0 | 1 | 0 |
| 1760 | 02004A = 7000 | 106 /149 | 91 | 34733 to4829 | 2 | 1 | 0 |

The actual profile pointers are ADB08E, ADAFEB and ADB08E, respectively.
`native-catch-attribution.json` retains every declared scratch value and the
actual register trace. Allthree calls leave actor/controller/context/profile
and order-list words unchanged throughAE10. RNG07F6, attempt0904 and the
documented volatile DP words match exactly.

ADFC-AE07 temporarily puts the receiver in96 for F5E4, then restores96;
8E stays the receiver. F02D preserves carriedX. Natural F02D entries carryX
3CEB,000A,0000, so callers must not assume that X always identifies an actor.
F5E4 loads X from96 and leaves that receiver inX even after AE07 restores96.
AE10's existing source reloads96 before actor access. This new typed API
models the96/8E words, not a complete CPU register or stack machine.

The capture records numeric CPU D/X/Y/PS at all boundaries; the verifier
checks native16-bit A/X width with decimal mode off, the AD3D receiverY contract, AD83 Y42,
AD8D tableX, F02D X restoration and F5E4 subjectX. It does not claim parity
for every CPU flag, A, SP, DB, stack byte or every unmodeled volatile register.
The seventeen DP words compared are00/02/51/8E/92/96/9A/9E/AA/AC/AE/B2/B6/
BA/BE/C2/A6. F5E4's saved B6/BA/BE/C2/9A/A6 remain unchanged atreturn;
AA/AE/AC/B2/92 reflect the actual scan, including early stop order.

## Preserved source details and remaining boundaries

F061/F063 uses CMP/BPL without F34F's extra equality branch atF37F. The new
F02D translation preserves wrapped subtraction and its distinct equality
behavior; it does not call the old general target-direction helper.
F5E4 scans the original spatial list forward and then backward, skips the
ball and same-team entries, reverses on the forward X miss atF697, and returns
clear on the backward X miss atF6F4. Y misses continue scanning. It must not
be replaced with a normalized scan of every actor.

Allthree natural calls take the eligible-distance path, use one nonzero-seed
RNG call, then F02D and an obstructed F5E4 result, continuing atAE10. Natural
evidence does not cover the early distance exits, profile ranges below76 or
atleast92, zero RNG seed, rejected axis gates, direction equality/extremes,
zero vector, clear lanes, wrapped coordinate extremes or AF66.

AF66 is implemented only to the exact first missing childB468. It reads the
raw six-byte band offset atAFA6, adds36, writes receiver timer60 and exchanges
96/8E. Atband30, the actual table address reaches AFC4's opcode bytes A6 8E,
yielding8EA6 rather than an invented sixth data row. This source-derived edge
has a PC comment but no controlled or natural execution witness here; it is
not presented as a naturally confirmed bug. B468, AF87's pointer restoration,
receiver mode14, source flagsOR4 and the saved51 restoration remain unexecuted.

The successful natural route stops beforeAE10, whose separate aligned module
already implements the next choice. This checkpoint does not wire modules
together or establish the entire AB2D initializer. Other earlier stops,
including ACA9/AFC4, B3BD and the AF4D stack epilogue, remain distinct closure
work. No normal human play is enabled.

## Integrity and first failure

The strict verifier checks exact JSON types/ranges, all artifacts and source
identities, executable/ROM/script arguments, exact route environment, supported
runner limits, native clock anchors and raw/event agreement. It compares
private settings/home/save attestations before the isolation helper can
mutate a copy. It requires all source-called children in order, including
balanced missing-pair and duplicate-pair rejection. The C response must have
every declared row, field, vector and integer. Stderr must be exactly the one
loader success line tied to the actual immutable asset-pack argument; missing
or extra diagnostics and noninteger process results reject.

All93 final mutation cases reject. They extend the accepted v2 integrity and
diagnostic recipe with actual-indirect-read, source-child-order and explicit
observation-mode guards. These are implementer-run tests, not independent
acceptance. The earlier83-case and88-case runs remain retained with their
exact verifier and test versions.

Independent review of the earlier pose checkpoint identified the missing
decimal-mode precondition before this catch freeze was written. ADC/SBC here
also requires nativeD=0; the routines do not execute CLD. The final verifier
requires PS&38=0 at every catch boundary, explicitly bounding this binary C
translation rather than silently changing the original decimal behavior.
`decimal-domain-v1/report.json` preserves five altered-metadata examples
accepted by the pre-guard catch verifier and rejected by the final verifier.
The native bytes and C outputs remain unchanged. All actual catch boundaries
have D=0. Decimal-mode native execution is not implemented or witnessed here.

No C/native mismatch occurred. The first verification attempt failed before
C ran because an inherited local integer named `pointer` shadowed the new
bus-pointer helper. Its failed report/log and exact verifier remain preserved.
The helper was renamed `long_pointer`; initial successful output and the
later stricter output are byte-identical. The exact initial successful
verifier was reconstructed from the preserved failed version and checked
against its recorded SHA, including original line endings. C/module/probe
sources were not changed for these verifier fixes. The original private
`/W4 /WX` build is clean; its log, binary and objects are frozen.

Probe SHA-256:
`1b807dc1d297ab1074a4e4ad712851d24b83fa4b13b8a7cbc506e88dc985a3c2`.
Final verifier:
`1f3e1cdf2fb286db29733adf5a360dc54dfde66a90618132a6ee538068489c37`.
Final mutation tool:
`80821f35abd4967d4b99f2e270343c7bb8095223bc90df50ccf5587c0b8151f8`.
Reports: `selection0-final-v2.json`, `selection2-observed-final-v2.json`,
`mutations-final-v2/report.json` and `native-catch-attribution.json`.
