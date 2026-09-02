# Frontend Route Checkpoint

This checkpoint covers the normal Exhibition route from Team Select through
Player Setup and the pregame presentations to Tipoff. It does not use a save
state, seeded WRAM, canned controller state, or a pre-rendered transition.

## Native comparison

The isolated `native-frontend-route-v1` capture used a normal power-on and
ordinary three-frame Start pulses. Its executable SHA-256 is
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`;
the ROM SHA-256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
All setup frames 385-900 were captured consecutively with frame skipping off.

Team Select enters at frame 451. Its reveal begins at 519, exposes the clipped
BG3 name/rank rows during the source's vertical release, and settles around
570-575. Those transient fragments are therefore preserved original behavior,
not classified as a port defect. Start at 650 begins the source-backed exit at
651. Per-layer motion continues through 672, brightness withdrawal occupies
673-700, and frame 701 is black. Player Setup remains black through 767 and
begins its brightness/slide reveal at 768.

The C full capture starts at frame 160 so its persistent framebuffer is valid.
Start is pressed at 178, the outgoing compositor changes at 179, retains the
outgoing owner through 228, and is black at 229. Player Setup first appears at
296. This matches the native +118 press-to-first-pixel boundary exactly and
preserves the native 67-frame fully-black construction interval (229-295 in
the C route, corresponding to native frames 701-767).

The early destination pixels are intentional construction, not a port-only
glitch. Consecutive native frames 768-769 expose the same partial right-edge
Player Setup layers as aligned production frames 296-297. The regression now
locks RGB hashes for the outgoing frame, first motion, last outgoing owner,
first/last black, and the first two destination frames.

## Controls

The production `NbaGame` caller accepts Left once from the default right-side
controller assignment to leave Player 1 centered. The resulting native
controller selection is `1`, and the controller runtime probe verifies zero
human controller counts for CPU-vs-CPU. Start skips MATCHUP, RATINGS, and the
complete lineup presentation. Left/Right remain the lineup card navigation
controls described by the ROM; A no longer advances one card.

The deterministic skip route also locks the visual handoff into Tipoff:
matchup, ratings, multiple lineup cards, black frame 595, and the first court
brightness frame 596. This is a production-route regression; exact native
timing for every skip edge remains separate evidence.

Run `tools/test_frontend_route.py` for this combined route and
`tools/test_player_setup.py` / `tools/test_player_intro.py` for the existing
asset, layout, cadence, and audio regressions.
