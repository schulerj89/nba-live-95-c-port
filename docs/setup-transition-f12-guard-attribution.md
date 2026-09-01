# Setup transition F12 guard attribution

`tools/test_setup_transition.py` includes two C-only F12 debugger checks. They
are renderer regressions, not claims of native SNES equivalence. Their complete
RGB hashes include the directory line at y19..26, so appending a resource to an
otherwise byte-identical pack changes the displayed count and therefore the
full-frame hash.

The old expected asset126 hash
`9c9a026b488b28c0317d9dacc47bbef9372db110d142634c8e692cf0a4c133fa`
was correctly bound to the 263-entry indexed-intro pack
`c7b90d9347c257e0746da7a6d5595e603ffd9d3a026666fe6e62c4f483e75a92`.
It became stale when resource286 was appended. The later literal-pose pack
appends optional resource287. Neither addition moves resource126, whose
65,536-byte payload remains SHA256
`0d25909881fe03449acf046c2d3a8cfaa64172f596864c3523c08806b581f89d`.

| Pack | Entries | Added ID | Asset126 F12 RGB | Asset128 F12 RGB |
|---|---:|---:|---|---|
| c7b90d93 | 263 | baseline | `9c9a026b488b28c0317d9dacc47bbef9372db110d142634c8e692cf0a4c133fa` | `919da3c071ad245b83ad027651fa2beb557038c19202f492aa6732ce124d85d9` |
| f564c296 | 264 | 286 | `d5fec70926527df285c1758c4e835c611efebbc2cb273dc180f79e202add2de3` | `aa1c60ea0d9792c0feaaaddd3351888c1bbb195f57643ae7d7db0d45912ca0ea` |
| be2d761e | 265 | 287 | `9cde8f84567fe618b4e40c6b4926b89a9060be81330e738ce42ad7b790402ccf` | `ee84e706b2bf4b958aa160754cbbb8802083b78f1d9823ce927aa5aed3f30fd3` |

The 3db96b2 executable
`7de9771a70c9fce918b9244e62f14e3bb83a2e46b4f39385a2dba7d30fb61035`
and 74a74ba executable
`fb52a4cdf15e849b0d8ad209649c5f3ad50a8e70bd0feee37a48b4b5c52ae040`
produce identical F12 hashes for each pack. Comparing f564 with be2d confines
the difference to `(56,19)-(63,26)`, the final count digit. Hashing y27..223
also produces one stable value per asset across both executables and packs.
The regression now checks both the count-specific full frame and that stable
canvas, preserving the metadata check without confusing a directory append
with changed Setup graphics.
