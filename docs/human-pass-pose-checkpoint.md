# AF1D pose, attachment and state commit checkpoint

This bounded slice starts at the actual `86:AF1D` caller, refreshes the actor's
pose through `87:AEC3`, attaches ball X/Y through `87:B649` and its `87:B832`
child, writes ball Z in the caller, then executes `86:AF30-AF4C`. It stops
before the first stack pop at `86:AF4D`. It neither completes the whole pass
initializer nor enables human play. All ten earlier freezes remain unchanged.

The new `nba_human_pass_pose` module/header and nine supporting files are
separate from the frozen action/aligned files. Build output, native captures,
reference dumps, test reports and the patch/freeze are private under
`build/human-pass-pose` in `completion-controllers`. No production manifest,
root worktree, old initializer, session ABI, commit or push changes here.

## Implemented contracts

| API | Native contract |
| --- | --- |
| `nba_human_pass_pose_resolve` | `87:AEC3-AF74`: literal upper/lower phases and selected bank84 descriptor lists; publishes resources, mirror flag, cached states/phases and facing. No cadence advance or phase modulo. |
| `nba_human_pass_pose_offset` | `87:B832-B952`: point0 for scratch00=0, point1 for any nonzero word; original sign extension, word-width mirrors, signed midpoint and X/Y/Z offsets. Preserves actor state. |
| `nba_human_pass_pose_attach` | `87:B649-B669`: forces point0, saves previous ball X at0922, then writes only ball integer X/Y. |
| `nba_human_pass_pose_prefix` | `86:AF1D-AF30`: executes resolver and attachment, then writes actor Z plus scratch04 to ball Z atAF2D. |
| `nba_human_pass_pose_commit` | `86:AF30-AF4D`: actor mode15, flagsOR6, and live state2 when the wrapped CMP80 result is negative. |
| `nba_human_pass_pose_prepare` | The complete AF1D prefix plus AF30 commit, stopping beforeAF4D's stack epilogue. |

These are direct source translations with typed actor, ball and scratch words.
They do not call the earlier conservative whole-initializer implementation,
load expected states, interpret opcodes, or reassign an owner. The source
manifest does not include this module, so no production human path is enabled.

The supported asset/input domain is states0..56, facing0..8, literal phase
lookups inside bank84, and attachment indices0..82F. Invalid or out-of-pack
addresses fail; they are not replaced with fallback resources. The module
reads the existing version6 `NBPANIM1` asset. At probe startup, all49,536 bytes
of bank84 and eight attachment arrays are independently compared with the
original ROM. No later native pose/resource/offset seeds the calculation.

## Original source and natural witnesses

Original ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen executable:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Immutable asset pack:
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

Fresh Ghidra 65816 and recompiler output covers `86:AF1D-AF66`,
`87:AEC3-AF75`, `87:B649-B66A` and `87:B832-B953`. The reference also retains
bank84 and the eight original attachment arrays used by the asset pack.
`reference-v2/manifest.json` binds exact ROM ranges, source/tool hashes,
commands, logs and artifacts; SHA-256:
`76c3652f7b581c4b89f1d0c4457e8d1a57ceb9a493426ad03feca20455361cd1`.
The earlier reference-v1 is preserved but is not normative: it unnecessarily
included a partial following B953 routine. B832 computes height inline and
does not call B953. The corrected reference ends at B832's actual final RTL.

Left and right captures ran sequentially in separate fresh isolated Mesen
processes. Each used private settings/home/save directories, fixed zero RAM,
the ordinary Player-screen route and ordinary directed/neutral B input for
2,400 court frames. There are no controlled inputs, ROM/WRAM/PC patches or
after-state injections. Left selection0 uses two released left taps from the
fresh `[2,1,1,1,1]` controller selection; right retains selection2.

| Route | Raw events | AF1D calls | Resolve/offset/attach/prefix/commit/combined | Compared values |
| --- | ---: | ---: | ---: | ---: |
| left0 | 611 | 16 | 16 each | 168,096 |
| right2 | 361 | 9 | 9 each | 94,554 |
| total | 972 | 25 | 25 each | 262,650 |

All comparisons pass. Native prestate is captured independently for every
component and the combined AF1D call. Each response compares all declared
outputs: all11 actor records (including ball fractions/velocities), five
controller records, both canonical contexts, profile table, order list,
14 globals including0922, and DP00/02/04/06/47/49/AC. The actual CPUX/Y/PS
are recorded as additional metadata: B832 restores both entry registers,
and AF30 enters with X equal to the actual actor at96. These checks prevent
assuming actor ownership from a stale carried register.

The native runs cover all facing directions0..7, both lower-table selectors
(12 ordinary,13 alternate), upper states2A/2C/2F/30/31, five lower states,
and lower phases0/1/2/5/6/7. All upper phases and actor Z values are zero.
Twenty-one calls start with live state0; four start at inbound82. All25
B649 calls use point0. Facing8, point1, extreme signed-byte values, wrapped
coordinate extremes, invalid addresses and nonzero actor Z are unwitnessed.

Capture manifest hashes:

- `selection0-v1/manifest.json`:
  `ae07e3d2f3149d87a69dd0a9d14158f65b121f0cf42414db1bfe8b30ff7c2484`
- `selection2-v1/manifest.json`:
  `66774dfc7518f9c1a819d2021cda5eb6be8326f72d282cbd77ba7392949aa7c6`

`native-pose-attribution.json` records each component's actual actor, scratch,
ball and previous-X state. B649 leaves ball Z and every ball fraction/velocity
unchanged; AF2D subsequently writes Z. Seventeen calls follow the earlier
alignedAF1D boundary, five follow B00B's AF1D boundary, and three arrive later
after the earlier AD3D stopping boundary. Capturing those three actual AF1D
entries does not implement or validate the intervening catch branch.

## Preserved original details

`AEFE/AEFF` uses ASL carry for the direction index; within facing0..8 it is
zero. The resolver reads literal phases rather than wrapping by frame count.
`AF6F` preserves cached facing52 when facing is8. All are retained in code.

`B83B/B83D` toggles mirror masks1/2 when actor flags28 is negative. The original
first sign-extends attachment bytes to16 bits, then negates them atB869/B86C
andB8AA/B8AD. The new module keeps that width, including the source-derived
minus128 to plus128 edge, without claiming a natural extreme witness.
`B8B1/B8B4` floors the signed midpoint through CMP8000/ROR. It is not truncation
toward zero. These source quirks have exact-PC comments; no presumed original
bug is silently corrected.

`B651/B654` stores the old ball X, before calculating the new attached X.
`B649` does not write Z; `AF25-AF2D` does. `AF42/AF45` tests the negative flag
of wrapped subtraction, not a signed comparison of operands. Ball fractions,
velocities, owner and controller assignment remain untouched by this slice.

## Verification, failures and limits

The strict verifier checks exact JSON types and word ranges, all artifact and
source identities, executable/ROM/script command, route environment, runner
limits, clock anchors and raw/event consistency. Private settings, Lua home
and saves are checked before the isolation helper can mutate a copied record.
The C protocol includes exactly the expected loader stderr line tied to the
actual SHA-pinned pack argument, and every stdout field/row/vector/integer.
Earlier caller stages are structurally observed, never replayed under this
new probe's mode names. Zero pose comparisons cannot pass.

All73 mutation cases reject, including raw contradictions, source/artifact
omissions, word/type errors, clock shifts, missing pose/offset stages, CPU
width/actor/restore violations, complete C-output corruption and diagnostic
protocol changes. The tests derive from prior independent audit cases and
are expanded/run here by the implementer; independent acceptance is pending.

No C/native mismatch occurred. The first build emitted C4459 because a probe
asset-range table shadowed its raw-range array; the initial source/executable
and warning log are preserved. The corrected final build uses `/W4 /WX` and
passes without warnings. Initial comparison reports/probe and the initial
verifier are retained; final verifier cleanup removes unreachable earlier-mode
input code without changing compared words.

Final probe:
`d6f9b5410790a7403ebd234a674d2b2a588d6bfc6ca3b80fdd08eac44e86e961`.
Verifier:
`d20feb6bb0cf5eedb545caff4646594e20143f4e66661d5bf1ad3ef6c91c649b`.
Mutation tool:
`ca903f9d5d1e3a894a20ecf38a77f7da99513fe001b1a1458dc2d3e506fc9adc`.
Final reports are `selection0-final-v1.json`, `selection2-final-v1.json` and
`mutations-final-v1/report.json`.

The exact first remaining boundary is `86:AF4D` before PLA restores9A/9C,
BE/C0, BA/BC andB6/B8. This API does not synthesize or interpret that stack.
The existing capture observes subsequent pass resume/return only. Earlier
AD3D catch, ACA9/AFC4, B3BD and relative-negative routes still require their
own closure, along with the complete production human caller. No normal human
enablement, complete initializer return, or whole-game parity follows from
these component comparisons.
