# F12 and tip-off checksum attribution

The stale F12 and tip-off guards were reproduced from a clean private checkout
of `3db96b2edb2686eadd37235284a0e85ff9af2459`. The fresh 40-translation-unit
`/W4` executable is SHA-256
`7de9771a70c9fce918b9244e62f14e3bb83a2e46b4f39385a2dba7d30fb61035`.
The canonical ROM is SHA-256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

Two version-31 packs were exercised. The 264-item fallback pack is SHA-256
`f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`.
The 265-item active pack is SHA-256
`acc4a436c990fd3a7beab9dadab47d40690ccedf912f7db90e283264ed0f299a`.
A complete directory/payload comparison finds one difference: active adds
resource 287, a 2,144-byte `NBPDRAW1` payload with SHA-256
`2c561159b63e56e5e42a4d461a1f03bee65c1f7b94fcc5ee933349cbc66bff9f`.
All 264 fallback entries are byte-identical.

## F12 logo view

Asset 160 resolves to the same item index and the same 48x56 ROM logo in both
packs. The two F12 captures differ in 28 pixels, all inside the last item-count
glyph at `(48,19)-(55,26)`: the browser prints 264 for fallback and 265 for
active. Replacing that glyph with black gives the same RGB SHA-256
`3430e5ff2ca65cc16ab37e586a92bbcc717b65be0424b19dfc8a44032ad8e95d`
for both captures. The regression now validates the exact supported pack
shape and resource-287 payload, each full frame, and this shared masked canvas.

## Tip-off frames

The previous 90/170/220 hashes are reproduced exactly by fresh builds of
`dcb1eb8` and `8c97b5f`. They first change at `9c69275`, which binds the
source-backed scoreboard clock and panel lifecycle to the live render. Fresh
fallback builds of `9c69275`, `facd818`, `744809a`, `9d5bc10`, and `3db96b2`
all produce the same replacement anchors:

| Frame | Fallback RGB SHA-256 | Active RGB SHA-256 |
|---:|---|---|
| 90 | `576f1a252b9f73060bd1d1023045587e391dd4b1b220aaa1a22d9c6ec7b047a3` | `814957ebbb1717ae86f370e9a32a90d35e58dca28fec95ebd5039f7f00c148f9` |
| 170 | `f24ba01e30a5b3b62d586f2786305e1ba3195ceda7e6d5894543b49b4ae86034` | `95ecfc14520f73d92370d7dddad460866f96227df3b2c562dfef16f7ee1885a4` |
| 220 | `45ee1c3fb42eb88322c0d1d9effa80c746256a80d4da536b68a2ca3e5cfd336e` | `45ee1c3fb42eb88322c0d1d9effa80c746256a80d4da536b68a2ca3e5cfd336e` |

With one executable and the byte-identical base entries, enabling resource 287
changes 64 pixels at frame 90 and 48 pixels at frame 170. Each delta is five
small disconnected components at visible player locations, consistent with
the literal head/jersey-order inputs documented in
`sprite-pose-runtime-adapter.md`. Frame 220 is pixel-identical. Gameplay state
and the court assets are unchanged by the optional resource. The configuration
aware guards therefore retain separate reviewed pose-render hashes instead of
silently replacing the old expectation.

This attribution establishes deterministic C-render behavior for these three
anchors. It is not a new native scanout proof and does not expand the accepted
scope of the HUD or literal-pose components.
