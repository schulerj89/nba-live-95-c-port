# Gameplay sprite and animation closure

Verified 2026-08-29 against the US ROM, retained Mesen witnesses, the headless
Ghidra census, the recomp gameplay extract, and fresh C gameplay captures.

## Corrected live-render divergence

`$87:A52C-$A5FA` can choose a presentation direction that differs from actor
`+$4E`: receivers face the ball and passers face their target. The telemetry
path already resolved upper/lower art for that draw direction, but
`nba_tipoff_render` bypassed the resolver whenever action resources at actor
`+$2A/+$2C` were cached. The renderer therefore combined movement-facing
torso/legs with presentation-facing head, number, flip and attachment order.

The renderer now calls the same `actor_animation_resources` boundary used by
telemetry. Cached action resources remain valid only when draw direction equals
actor direction; otherwise `$87:AFA2-$B053`'s appearance-aware descriptor path
resolves the requested direction. The deterministic 1,200-frame comparison
changed 341 presentation frames without changing gameplay telemetry. Reviewed
anchors 1300 and 6954 changed; 600, 3480 and 6932 remained byte-identical.

## Asset and compositor closure

`animation_render_closure_probe` walks every non-null native descriptor for:

- all eight directions;
- both appearance variants;
- ordinary and tall-player lower-body tables;
- enough descriptor ticks to traverse every frame list;
- both uniform sides through the production head/number/body compositor.

The exact checked-in closure is:

| item | count |
|---|---:|
| Upper descriptor states | 52 |
| Normal lower descriptor states | 39 |
| Tall-player lower descriptor states | 39 |
| Unique resolved upper/lower pairs | 2,610 |
| Unique upper resources | 955 |
| Unique lower resources | 710 |

Five upper and eighteen lower state-table slots are native null pointers. They
are not missing asset-pack entries. Every non-null combination resolves packed
ROM art and successfully composes valid lower, upper, head and optional jersey
number layers. No Mesen screenshot pixels are runtime assets.

## Natural gameplay evidence

A fresh 1,200-frame CPU-versus-CPU run retained 12,000 actor-frames, all eight
directions, 32 naturally selected upper/lower state pairs and 389 naturally
drawn resource pairs. All 9,283 visible actor-frames had valid palettes,
resources and opaque lower/upper/head layers; every allowed jersey number had
packed pixels. Frame-by-frame review found complete uniforms and stable layer
ordering through tip, pass, receive, dribble and basket-area overlap.

The permanent 63,800-frame gameplay regression continues to check missing
layers, animation/resource consistency, ball pose points, OAM-origin jitter,
action families and reviewed pixel hashes. `build.ps1 -Test` now also runs the
descriptor-wide closure probe.

## Boundary of this claim

This closes packed player art, descriptor resolution, presentation direction,
four-layer composition and the CPU gameplay bindings currently reachable in
the port. It does not claim that unsupported human controls, substitutions,
free-throw orchestration or every unobserved native effect/indicator branch is
reachable. Those are feature/capture gaps, not known missing player graphics.
The controlled-player off-screen indicator beginning at `$87:A846` remains
outside CPU-player sprite composition.
