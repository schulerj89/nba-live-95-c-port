# Period role continuation v2

This separate implementation extends the frozen role prefix through `$85:BD0D`
rebuild and the `$85:BE06` planner within the period parent's `$81/$82` domain.
It does not replace the original 714-identity v1 packet. The two C modules link
together: v1 owns the initial scan/F34F/cadence gate; v2 performs its source
continuation and returns to the original paired caller. No production manifest,
main/audio code, timing inputs, or native data were edited.

`NbaPeriodRoleStateV2` contains 223 named typed fields: the original prefix state,
additional actor mode/assignment/reaction fields, context anchor/order bytes,
ball assignment/anchor distance, and carried source globals including RNG07F6.
The probe accepts one 450-byte typed before-state only, with magic5252/version2.
It has no expected-after or child-result inputs. Repeated advance at a boundary
is immutable; only FIRST_RETURN resumes. Its ordinary C API requires owned
current gameplay state; native snapshots are used only for isolated component
differentials.

The source domain is binary M=X=D=0 with DP0, live81 or82, canonical actor IDs
0..9 and team groups0/5, initialized46EB/476B contexts, ball0910=3EEB, and
reciprocal cross-context bijections in each of current74/base76/alternate78.
Those three assignment sets may differ. Context+49 contains five distinct even
indexes naming the opposing five actors. The API checks these conditions. It
makes no fixed cadence, rebuild, camera, owner, receiver, RNG, nearest-pointer,
mode, coordinate or threshold assumption.

Implemented source behavior:

- E1E5 calls BC07 with context46EB, then E1EE calls it with476B. Camera or owner
  side does not reverse this sequence.
- BD19 copies alternate78 for the entry context and calls B95C on eligible
  non-owner actors after64=2F. BD55 then BDB2 process opponent and entry contexts,
  copying base76/current mode84 and resetting low modes according to live82 and
  side0952. All real B95C calls occur in original order. BE03 alone clears09D6,
  after those three passes; an earlier cadence return does not clear it.
- B95E clears actor7E even when B969's live82/inbound0954 early return preserves
  actor60 and the RNG. Other calls measure against the physical ball3EEF/3EF3,
  advance shared07F6 via80CEE7, add RNG&78, and apply B9C0's wrapped CMP/BMI clamp.
  Wrapped abs8000, distance overflow, and the RNG zero recovery9146 are preserved.
- BE23 computes anchor distances for the opposite context. BE73/BE76 skips
  live-play mode/assignment cleanup for81/82. BF3C publishes09E2.
- BF89 uses the ball record's74 assignment to select the primary actor. It does
  not use owner XY, and it has no defense-context side check. Therefore ball74=0
  can select actor0 during either call. BFC7's boost comparison is unsigned BCS;
  the surrounding source comparisons usually use wrapped subtraction's N bit.
- C052 handles a negative owner. Negative receiver0946 enables nearest promotion;
  fallback scanning preserves the original last-equal-candidate tie rule and
  the no-candidate early return. C0B4 scans context+49 in literal order. Its09DA
  counter ends at0 after success; it does not preserve the earlier nearest
  actor pointer.

Explicit unresolved boundaries:

| Kind | Source stop | Meaning |
|---|---|---|
| RECORD_READ | BF51 | Carried09DE does not name one of the ten typed actors; stop before reading that record+74. |
| RECORD_READ | BFAB | Nonnegative ball74 is outside the represented even0..18 ROM table slice; record_pointer reports the table displacement. |
| RECORD_READ | C05C | Carried09DA does not name a typed actor; stop before reading mode+5E. |
| ASSIGNMENT_CHILD | BF5B / BF98 / C0DD | Stop before JSL BAE4 and therefore before any repair/displacement child effects. |

The assignment-child paths are not silently skipped. Initial actor assignments
are bijective and the81/82 cleanup gate preserves them, so ordinary represented
actors avoid these calls. A negative ball assignment can still reach BF98 and
is explicitly stopped. Arbitrary record aliases, live-play helper assignment,
BC07's remaining BAE4/BAB7/B9D2/BA1D closure, and the parent's D5DB/FBFF render
sort/publication tail remain outside this helper.

A material carried-state caveat is retained: all four native roles.before
snapshots contain09DE=00A5 and ball+8C=665. Their actual cadence12/rebuild0 returns
before the planner, so no native BF51 read is witnessed. Forcing a rebuild in
an isolated source case reaches the explicit BF51 boundary after all preceding
source work. The helper must not invent a nearest-offense actor or substitute
an unrelated C defense-wrapper value to pass that boundary. Further source
ownership of the aliased00A5+74 read is required for this carried state.

Evidence and limits:

- Original ROM SHA2562115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
  Literal paired caller and879C7B/F09A table bytes are checked. Relevant ROM range
  hashes are recorded in the native report; B95C..C0F5 includes excluded children,
  and the range hash is not a claim that all its instructions were implemented.
- Fresh `.analysis/period-roles-v2-build-v1` compiles both modules with /W4/WX.
- Four immutable enhanced captures match all223 fields atE1F7 (892 word/byte
  comparisons). These are the same controlled period-expiry captures as v1;
  they prove the early-return route and preservation of added fields only.
- 116 controlled source-only cases execute222877 original-ROM instruction
  decisions across509distinct PCs and55087 memory writes. They compare
  50398 typed boundary fields against C, including rebuild RNG order,
  distinct base/alternate bijections, negative cadence and camera, ownerless
  fallback, cross-context primary selection, wrap edges and explicit stops.
  The test-only bounded ROM diagnostic reads actual opcodes/operands. Every
  accessed data byte must belong to the typed input projection, except C0C2's
  ignored high byte before AND00FF. It is not a production interpreter, timing
  model, CPU-stack/register parity claim, or proof of normal branch reachability.
- 13 C input/domain rejection contracts and17 reachable verifier corruption
  tests pass. The probe also checks immutable terminal boundaries and resume
  restrictions on every case. Duplicate keys, non-integer-zero status, malformed
  words/metadata, missing/extra rows and native M/X/D mutations reject.
- Independent audit is pending. No full BC07/normal journey/audio/phase parity
  or production-integration acceptance is claimed.

Reproduction from the scheduler worktree (choose fresh output directories):

```powershell
& tools/build_period_roles_probe_v2.ps1 -OutputDirectory .analysis/role-v2-rebuild
python tools/verify_period_roles_v2.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/role-v2-rebuild/period_roles_probe_v2.exe --output .analysis/role-v2-native --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-1-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-2-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-3-ready1-children-v3
python tools/test_period_roles_v2.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/role-v2-rebuild/period_roles_probe_v2.exe --native .analysis/role-v2-native --output .analysis/role-v2-source
python tools/test_period_roles_protocol_v2.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/role-v2-rebuild/period_roles_probe_v2.exe --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 --output .analysis/role-v2-protocol
```
