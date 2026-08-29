# Human player edge-indicator differential

## Native boundary

`$87:A846-$A97D` clips a human-controlled actor rejected by the preceding
visibility gate and prepares the controller arrow submitted at `$80:B344`.
The routine deliberately has two coordinate windows: `$87:A846-$A874` uses
the broad viewport to decide whether the actor X receives the `-50` sentinel,
while `$87:A880-$A8E0` clips the arrow itself to the tighter 17..207 safe
frame. A top-only indicator retains actor X; left, right, and bottom paths do
not. This apparently asymmetric behavior is retained as a ROM quirk.

## Evidence and production binding

`tests/fixtures/draw-indicator-witnesses.json` contains 200 natural calls and
32 controlled real-entry calls covering four controllers and all eight edge
classes. No program counter, ROM byte, return stack, or child output is
patched. `tools/verify_draw_indicator_vectors.py` replays all 232 calls through
the production helper and compares resource, attribute, clipped X/Y, and actor
screen X; all outputs match.

`latch_player_screen_origins` calls the helper for a human-controlled actor
when the existing native visibility gate rejects it. CPU actors are unchanged.
The helper exposes the exact native resource IDs (`$0814/$0815`,
`$0817/$0818`, `$081A/$081B`) and attributes to the presentation state. The
resource bitmap upload/submission child remains outside this credited range.

## Regression gate

`build.ps1 -Test` rebuilds the vector probe and requires the complete 232-call
fixture to remain zero-mismatch. The fixture asserts its natural/controlled
census, so silently dropping a difficult edge class fails the build.
