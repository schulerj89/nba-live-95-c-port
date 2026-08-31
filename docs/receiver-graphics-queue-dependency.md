# Receiver pose depends on the graphics publication ring

This is a source/runtime dependency found during C2, not a request to replace the native value with a convenience flag. Bounded B468 and AF66 parent C replay currently match their checked native words; ordinary C gameplay integration remains pending this producer and the actual catch gate's input owners.

At `87:B7DA`, the pose helper loads Y=$0084. Its `B7E1 LDA $00A8,Y` therefore reads DBR:$012C. Fresh normal-game entries have DBR=$7E. The address is **not receiver+$A8**. In four right-route child calls the receiver+$A8 word is0 while012C is nonzero. This original address quirk must remain; use a source-PC comment when the live adapter is wired.

The original shared publication ring is `$0100-$02FF`, 64 eight-byte records. `$012C` is record5's length word (`$0100+$28+4`). Its value survives publication; consuming a record must not zero its bytes unless an actual source store does so.

## Direct producer evidence

Normal right selection,600 court frames, no injected state, `build/c2-native-012c-v1/alternate-writes.jsonl` records all matching writes. Before the observed B468 call atcourt463, the immediately preceding frame462 writes012C from the ordinary graphics producers:

| Actual writing instruction | Source value/address | Callback-reported PC |
| --- | --- | --- |
| `80:AD67 STA $0103,X` | bank word=$007E, X=$0028; overlapping high byte clears012C | AD6A |
| `80:AD6D STA $0104,X` | length=$0020, X=$0028 ->012C/012D | AD70 |
| `80:B7B7 STA $0104,X` | current0601 length, X=$0028 ->012C/012D | B7BA |

The write callback reports the post-instruction PC. Do not attribute the store to the next LDA/BNE. The first capture's optional `writer_012c_*` convenience metadata failed to normalize bus addresses for7E-bank writes and remained stale; preserve that defect. The complete chronological write log contains the actual bus addresses, values, clocks and register contexts. Producer attribution derives from that log and the pinned ROM, not the stale convenience fields. The new script normalizes only its lookup key, preserving full bus addresses in the log. This capture is a producer-attribution diagnostic, not a certified final protocol packet.

## Smallest useful shared owner

`NbaSetupPublicationQueue` in `include/nba_setup_scheduler.h` already defines the correct persistent512 record bytes, head35, tail37, budget39 and palette descriptors. `nba_setup_queue_publish` owns source821A-83CE mode1/FC/FD/D6 consumer semantics and leaves record bytes intact; unsupported queue modes are explicit. It is not currently a production NbaGame owner. The source-work codec/producer/header APIs accept external memory/bus state and have explicit immediate-empty-queue domains; they do not supply a whole-game ring lifetime.

Use **one persistent publication-ring state shared by scene/graphics producers, NMI consumer and gameplay readers**. Reuse or promote the existing queue type/API; do not create a second NbaTipoff boolean or a shadow012C word with independent lifetime. Root should own placement in shared scene/session state. The B468 adapter needs only a read of `records[0x2c..0x2d]`, but the record's value must be produced by the source ring order.

The minimal closure before live B468 acceptance is:

1. Preserve initial and carried ring bytes and cursors across the actual boot/menu/new-match path. Do not initialize a fresh queue at B468 or assume record5 is overwritten without a source-proved route bound.
2. Translate/invoke the reached ordered graphics appenders, includingAD57-AD84 andB79E onward, with exact widths, overlapping stores, source0601 and `tail=(tail+8)&$01FF`. Their inputs come from the real graphics operation/CHR selection and order, not the final rendered RGB image. Include other reached producers that can claim record5 or move tail; rendering only the actor being passed to is insufficient.
3. Run the actual consumer boundary using the same queue bytes/cursors and correct source publication budget. Modes not owned by `nba_setup_queue_publish` must remain explicit. It may be possible to prove a bounded queue-data result without full scanout timing, but that is a separate proof, not implied by this child replay.
4. Compare source-produced ring bytes/cursors and012C at continuous ordinary B468 entry boundaries, including record wrap/reuse, empty/nonempty, exhausted budget, scene changes and pause/restart. Then supply that state to the typed B468 input. Do not substitute actor+A8, constant1 or the last observed native word.

The current D1 ordered graphics pass is the relevant producer work; culling alone does not close this dependency. S1 remains responsible for absolute CPU/NMI/scanout phase claims. C2 can proceed independently with typed parent/child composition, exact input adapter tests and source/control vectors. There is no reason to roll back the accepted C1 correction or enable human gameplay early.

## Separate AF66 parent prerequisite

AF66 leaves E0/C2 identifying the passer while temporarily changing96 to the receiver. Thus accuracy86:AA6A uses inherited passer profile/stamina but receiver+B2/+6E. Both natural AF66 captures confirm that split. The preceding AD3D gate also deliberately reads `[DP00]+$42`, already documented by the accepted catch module; it is not a profile-pointer synonym. A production gate must compose those source inputs and the ordered lane list. `selected==special_actor` and the historical whole pass initializer do not establish the actual AF66 branch. No broad shared+56/+58/+60 alias rewrite has begun.
