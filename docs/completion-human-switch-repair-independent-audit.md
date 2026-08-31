# Independent switch repair review

PASS for the bounded repair frozen by
`completion-owner/build/switch-repair-freeze-v1.json`, SHA256
`e51d84086dbc3980cfc7d6d9da6d78aef1d34a225780868775b6de9db9aa6adf`.
All30 file and39 link-object identities were independently checked. The
original switch11 files, original captures and prior rejection evidence remain
unchanged. This accepts neither production human enablement nor whole-game
caller/scheduler behavior.

The repaired helper source SHA256 is
`541c81f4b928fb259483e38b8276f8630ad7136a33843e65ee1cab63fd0e78de`.
I inspected its diff against the frozen dependency and original F34F bytes.
Only the two comparisons change: F37D accepts equal or negative wrapped
subtraction; F399 accepts negative wrapped subtraction. Existing direction
table, magnitudes, shift wrapping and distance result are unchanged. The
comment correctly identifies the native PCs and direction6/distance8000
counterexample, without asserting natural reachability. This preserves the
original unusual arithmetic instead of replacing it with conventional signed
comparisons. The switch module itself is byte-identical.

I freshly compiled the repaired helper and unchanged switch module/probe
with MSVC /W4 /WX in `build/switch-audit-v2/compiled`. Other linked objects are
the auditor's earlier freshly built accepted controller objects, read-only;
the old helper object is explicitly replaced. Fresh switch probe SHA256:
`63d32da8a4cfb32285164165de09c7575826dbe47158a2862287b6254310b312`.
The unchanged576-case independent direct-helper guard passes all cases.
The unchanged controlled7936-byte counterexample now returns RETAIN with the
source assignment preserved and target assignment untouched. It previously
returned TRANSFER. Both original native captures retain all68,480 comparisons
across40 calls,32 transfers/eight retains, and the observed native directional
tie at entry95/event98.

Repaired verifier SHA256:
`2858ad3b371c813c85b5a1fb47b7bfae75f7172987047ed66a4e462e80469515`.
Its new16-bit metadata domains and raw-event cross-checks match the pinned Lua
recipe, including the diagnostic actor label being DP C2 rather than DP96.
The direction check follows090C's actual word address and the capture'sFFFF
fallback. Player/court clock origins2697 and4590(left)/4390(right) agree with
the immutable source revision, route and raw captures; frame-to-court and
completion bounds validate diagnostic provenance only. They are not production
phase inputs. Existing exact source/command/environment/isolation/artifact and
typed persistent-output checks remain in place. All35 original and38 unchanged
independent integrity tests pass, including rejection of the previous15
accepted corruptions. This is bounded verifier testing, not an exhaustive
proof of every intermediate diagnostic event.

The global helper is shared, so the parent's broader caller regression suite
remains relevant integration evidence. This component review directly checks
the original source arithmetic, captured switch prestate replay and prior
failures; it does not claim to have rebuilt/replayed every production consumer.
Ordinary-coordinate captures remain unchanged, and no full-word extreme is
presented as a natural in-game witness. Count>=5, multi-controller behavior,
stale A6 reachability and unrelated human actions retain their earlier limits.

New evidence is under `build/switch-audit-v2`: `freeze-recheck.json`, fresh
build logs, `selection0-fresh.json`, `selection2-fresh.json`,
`direction-arithmetic.json`, `controlled-extreme.json`, `local-tests`, and
`independent-integrity-fresh/report.json`. Early auditor command attempts
misread the link-object metadata shape and invoked a not-yet-built probe;
those failed reports remain separate from the fresh passing replay.
The original rejection document remains historical evidence.
