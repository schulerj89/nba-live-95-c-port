# D1 draw-state consultation and implementation boundaries

On2026-08-31 the existing Max task `01a05634-5316-78c0-bb36-f9cdfd3b562e`
returned read-only advice on the blocked ordinary draw/state integration.
It did not implement code, build, capture or change settings. These source
findings guide implementation; this document is not acceptance of unwritten
code or a native timing claim.

## Persistent owner and source order

D1 can proceed before complete S1 if it implements the wired source pass and
keeps native cadence unclaimed. Carry one12-pointer permutation, depth+68,
screen+6A/sentinel state and actual basket position. FBE9 resets the permutation
at86:DA89; scene/period changes must not invent extra depth or order resets.

The ordinary87:A357 pass clears its two deferred queue counts and calls80:AC1B;
then executes the reached cursor overlay and live24-bit180B callback. Normal
80:800C is RTL, while87:A046 is actual code. A3B6..A435 prepares/culls all12
records in reverse carried order; A43E performs FC80's single11-comparison pass;
A47A submits in reverse new order; A7D5/A7F9 drain queues; A841 calls AC89.
Ball and basket bypass player culling. Submission must use the actual+6A=-50
sentinel rather than a second independently evaluated host predicate.

Each actor reaching A5FA commits+28 bit2 at A609 and a WORD at DP47 at A61E
in source order. DP46/47 overlap: one byte owner for46/47/48 is needed; deriving
47 solely from46's high byte loses byte48. Remove the current fixed final-actor
scratch patch. If no actor reaches that store, no replacement store occurs.
The part child's returned B8 becomes actor+AA at A6B2. Draw direction must not
overwrite actor+52 merely to make rendering convenient.

## Concrete port defects to repair and verify

-87:A419..A42D sends both depth>=288 and depth<-20 through depth-minus-Z,
  using wrapped CMP's N flag. The old C helper discards the low-depth branch
  and uses host signed comparisons. Root is implementing this narrow wired
  correction first, with original instruction tests and full C trace comparison.
-A58A/A58D compares the actor pointer with0940, the camera subject, not the
  possession actor. Current `actor_draw_direction` uses the wrong owner.
-A59C..A5A2 loads actor+88, logically shifts right and stores AA before jumping
  A5BC. It does not use an F02D stale-AE path. The contrary port comment and
  ignored upper-state/anchor-direction inputs need correction, not an
  original-bug label.

The minimum reached OAM closure includes180B callback, A846 indicator, F02D,
80:AD92, camera-subject AF1E with ball B0FF interleave, B344/B346/B348 resource
and flip logic, and AC1B/AC89 begin/finalize. Individual ordered pieces must
retain resource, attributes, clipping, shared cursor, high bits, reservation,
capacity and finalization. Rejected/clipped pieces consume no slot. ACC2 cache
or allocation descendants may reject a piece or change tile base. Deferred
jersey queues are real glyph work. Current whole-actor ARGB planes and fixed
actor/ball/basket indices do not satisfy this closure. Head order uses DP51;
number order uses DPC0 movement direction. Extend the wired resource helpers
instead of adding unused alternatives.

Exceptional FBFF calls at86:936D,86:E1FB and85:C366 share the same owner, with
gaps7/3/1, recomputed comparison keys and wrapped N/equality behavior. Expose
the accepted leaf if needed; do not reuse the entire period wrapper at all
sites, because that adds D5DB and an E1FF frame increment. The actual period
tail remains D5DB→FBFF→one084A/084C increment.

## Acceptance and cross-track dependency

Compare all12 order/depth/screen values, ordered status and DP writes, live actor
and ball outputs, ties/wraps, final-eligible/no-eligible cases and the next
scratch/RNG consumer from the same carried bytes. Re-rendering a completed
frame must not execute the state pass again. Verify ordered OAM piece identity,
attributes, indices/high bits, clipping, allocation rejection, stale tails and
buffer swaps. C journeys must reach ordinary and exceptional passes and period
return without seeding capture state. S1 later supplies real NMI/master/upload
visibility; A357 has direct callers85:8EE1,86:8323,83:ECE1, not merely an even
host tick.

Controllers' subsequent ordinary-input trace attributed the B468 DBR012C read
to the graphics transfer descriptor at0100+28+4. Native callback PCs are after
the writing instruction:80:AD70 corresponds to AD6D STA0104,X, and B7BA to
B7B7. The child must read that canonical queue byte owner. A receiver+ A8 value
or fabricated nonzero input cannot replace it. This makes the reached graphics
queue producers/consumers a runtime dependency of C2; before-only child replay
can still be verified independently. The first capture's stale per-boundary
last-writer convenience fields remain rejected; raw chronological writes are
retained and a corrected new revision is required.

C3 still requires canonical+56/+58/+60/B468, full channel copyback, roster carry,
09DA..09EC aliases, selected length0A0C, context28 and C6 delta. No legacy foul
dead-ball/substitution shortcut is accepted. Persistent graphics state belongs
outside the scene union; per-pass deferred counts reset at the original A357.
