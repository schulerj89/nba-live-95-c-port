# Raw ROM sprite compositor differential

## Credited boundary

`$80:B344-$B37B` selects the raw resource and establishes the descriptor part
count and origin. The portable equivalent resolves that identical resource
from the asset pack instead of allocating transient SNES VRAM. `$80:B37C-$B529`
then walks the descriptor, clips every part, applies horizontal mirroring,
writes low OAM, and packs signed-X/size bits into high OAM.

## Native evidence

`tests/fixtures/raw-sprite-compositor-witnesses.json` retains 2,000 natural
real-entry calls (`$80:B344` and the direct `$80:B348` entry), 25 resources,
both flip paths, 43 distinct input tuples, and 3,492 emitted parts. The
capture pairs all calls without orphan returns. High OAM uses a descending
four-slot cursor, so its two-bit fields are decoded in reverse slot order.

The verifier compares resource selection, descriptor part count and every
portable observable owned by the loop: signed X, Y, small/large size, flip,
palette and OBJ priority. The transient VRAM tile number and its name-table
carry bit are masked because the C port uses asset-pack pixels directly; those
hardware-only outputs are explicitly not claimed as equivalent. All 3,492
parts match. Existing player
composition tests separately cover 5,568 team/roster/direction cases, while
the live court endurance test proves the renderer consumes these resources.

## Regression gate

`build.ps1 -Test` rebuilds `raw_sprite_compositor_probe` and requires the
2,000-call witness to remain zero-mismatch, with at least 25 resources, both
flip paths, 40 distinct input tuples, and the exact native output count for
every call. Missing resource records fail closed.
