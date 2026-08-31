# Source draw direction and actual caller correction

This candidate corrects three port errors in the existing draw selector and its
actual `actor_draw_direction` caller. It does not implement the complete original
draw pass, attachment/OAM state writes, graphics queue or native scanout timing.
Production base is378900b; the separately accepted two-image test migration is
cherry-picked as8fc8261 without changing production source. All other production
logic is unchanged.

## Original branches retained

-87:A58A/A58D compares the actor pointer with camera subject0940. Mode14 takes
  the ball-facing branch when they differ. The old possession equality test
  chose the wrong owner and condition.
-A59C..A5A2 loads actor+88, logically shifts the word right once and goes
  directly to A5BC for upper states20/21 when earlier target branches do not
  apply. It never calls F02D with stale AE. The false original-bug comment is
  removed. Candidates>=8 retain movement direction; do not mask them to7.
-A5B6 calls85:F02D, not F34F. F02D's first slope comparison takes strict
  wrapped N; F34F additionally swaps on equality. For delta(0,1), F02D gives0
  and F34F gives1. The private draw quantizer preserves F02D without modifying
  the global target/distance function or contact callers.

Mode8 still takes priority, with status bit8 ahead of bit10. Valid mode15
receiver targeting takes priority over ball and upper-state branches. The
source turn-selection rule still advances by two directions for intermediate
differences and immediately adopts adjacent/wrap-adjacent directions.

## Bounded independent source proof

The test-only original-ROM executor covers87:A52C..A5FA and its85:F02D child,
reading instructions and tables from the canonical ROM. It derives expectations
before invoking C and imports no C outputs. It compares direction only, not
registers, stacks, OAM, original cadence or arbitrary scene state.

Both root and the independent auditor reproduce240,192 original-instruction
cases with zero differences. These include all65,536 anchor words at three
current directions, full-word coordinate edges/random cases, mode/status/upper
priority, camera versus independent possession identity, and actual static
caller/receiver alias cases. The fresh unchanged source control fails7,098
identical cases. Eleven malformed binary inputs reject; empty-stream and
same-actor receiver positive controls pass. The probe asserts the entire input
NbaTipoff remains unchanged by its read-only caller.

The supported movement direction domain is0..7. Existing caller mode, upper
state and anchor fields are bytes; its probe rejects larger values rather than
silently truncating them. The direct leaf's upper and anchor fields support
full words. The actor-only receiver domain is-1/0..9. Same-actor receiver XY
aliases actor XY; separate target coordinates are ignored. Unusual raw words
outside those caller domains are not certified by this correction.

Fresh caller build is39translation units including the real nba_tipoff.c as
the fortieth source body, MSVC /W4 /WX. The initial v1 build failed on probe
word-to-byte assignment warnings; that failed log is preserved and no v1
executable exists. The corrected v2 build succeeds; production source was not
changed to silence those probe warnings.

Independent source review: `checkpoint-qa-20260831/build/d1-direction-source-review.md`,
SHA2565a88e57766e8f49a1e33a16d0aefe0f4b39053a0dfb5499a9b54436a6ca08fc3.
The two independent oracle/test tools are copied byte-for-byte to tools.

## Real C loop and visual attribution

Fresh40-source CLI SHA2567a972a56a9b710da7010d814df0dc46c3a456591eabb507401061b8e7aedb3fe
completes63,800frames with HUD packf564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09.
Trace SHA2561752bc8abe429b4a0de91acf0aa9310b90b56c98a09aa0c86ff9137abab2d211.
Every field of every row was compared to the accepted C1 trace:1,858rows differ,
solely128actor draw-direction/resource and appearance field paths. The first
change is receiver actor4 at306. All other serialized state, including actual
ball/player coordinates, scores, clocks, contact/possession and RNG, is identical.
This excludes unlogged state and is not full machine equivalence.

Fresh before/after renders at306,31494,62980 differ by549,0,603pixels. Root
inspected306 and62980 pairs: changed player facing is visible; court/ball and
unaffected players remain intact. The31494 pair is pixel-identical despite
logged appearance differences. Exact commands, original BMPs, lossless PNGs,
logs and hashes remain under ignored `build/direction-images-v1`.

The user reported a ball-to-hands mismatch in the306 view. It is explicitly
OPEN and assigned to the gameplay agent with Max providing read-only advice.
The same passer3/ball values already exist in the baseline306; this candidate's
306change is receiver4. Passer3 is in mode15, unreleased, movement direction6
versus draw4, raw upper/lower332/1168 versus draw324/1154. Ball XYZ is-119,99,47;
camera0940 identifies passer3. This suggests attachment/resource ownership
needs checking, but no source cause or corrective offset is asserted here.
The narrow direction proof does not certify hand attachment or pass flight.

The complete regression rerun in `build/direction-broad-v1` exits1 after passing
gameplay/state/pass/receiver/motion and the two accepted600/1300 images, at the
same existing3480 RGB difference805bc5705a4faa6946bf484fe670614767a0f34a2465e490343c5cbff79e6662.
Later image/source-marker checks do not run. The separate contact guard passes
one real interruption/recovery with no unfinished episode. No whole-suite pass
is claimed. Final packet review,
combined integration checks and exact-commit checkpoint screenshots remain
required. Full draw ordering, DP46/47/48 writes, graphics descriptors and their
C2 receiver dependency remain open. Another worktree owns the separate contact
F02D correction; its patch is excluded here.

Subsequent source attribution by the gameplay agent and Max agrees that original
A4E1/A517 preserve raw+2A/+2C in D6/D4 before direction selection, and AF1E
receives those raw body resources. Rebuilding different body resources in the
port is therefore a separate demonstrated rendering mismatch. Its repair and
ball sequence/native checks are in the gameplay agent's worktree, not this
packet. Do not redirect physical attachment to incorrect reconstructed body
resources to conceal the gap.
