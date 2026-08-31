# Independent Setup regression migration review

PASS for the bounded test-only migration frozen by
`completion-owner/build/setup-regression-migration-freeze-v1.json`, SHA256
`20ecb74aaee35557f8de19cb2b6010471ea3b2760f1828d2878d99437fe5bdf0`.
All 23 frozen files were independently hashed before review and rechecked
unchanged afterward. No production source, original native capture, or expected
native RGB/PPU value was edited by this audit.

The exact frozen test, SHA256
`41c56b50552bb1172c4ca0a440b37a27b1dbf404151d2d7bd9846f0d5ce113cc`,
passed a fresh complete replay with 137 child-process runs and new private
auditor outputs. This review intentionally reused the frozen executable
`nba95_controller_owner_v2.exe`, SHA256
`b8cd19d4c0a8424d7a1d181dec71588df7037cd40f81404ecc3c14d34ed5a3a3`,
and candidate pack `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
It does not claim a fresh production rebuild: the 23-file test checkpoint does
not contain a complete production build closure. The test's source diff and
constant dictionaries were inspected independently of its success message.

The constant inventory changes exactly nine C image baselines and two menu
asset hashes, with no removed comparison keys. The two default images at
162/166 retain the previously reviewed Arcade/12-minute attribution. I decoded
the existing four-way old/new CLI and controlled-state BMPs independently:
all nine old images match new code with legacy state, all nine new images match
new code with fresh state, and only frames162/166 change, by1116 pixels each.
These are C state/rendering comparisons, not new native trajectory evidence.

I freshly reproduced all ten Main value images from the frozen executable
through real Simulation/3-minute configuration input. Seven differ from the
actual old BMPs (whose RGB hashes match the historical constants), exclusively
on the restored shadow row:30/33/33 pixels at y86;24 at y104;19/21 at y122;
26 at y140. The other three are unchanged. This agrees with the independently
accepted source span restoration; the ten native glyph hashes remain intact.

All seven historical cursor images and all13 Options opening plus10 Options
return hashes passed unchanged. A separate431-frame Options replay was checked
against the exact input CSV schema and every held, pressed, released, and
native word. Its15 isolated presses include A at168 and Start at302, with
explicit releases; it returns to Main with committed Simulation/3-minute
values. The route does not seed configuration or PPU state. It reproduces a
historical C phase using the C entrance's accepted-input interval and says
nothing about native initial-entry timing.

The adjusted sound assertions distinguish the two real Main adjustment events
from subsequent submenu sounds, including the blocked-slider no-new-event
case and the third-event volume comparison. The negative Season-tail test
poisons an actual ink row and compares the same journey with a clean pack.
Main reentry distinguishes committed edits from abandoned working edits.
Rules row13 requests preserve row12 clamping; this is separate from Main value
wrapping. The NBSPPU3 decoder accounts for the publication byte on VRAM entries
while retaining exact final-offset, state, and write-count checks. Asset145/155
publication changes belong to the separate source/publication audit, not a
new native oracle authored here.

Rules navigation frame compensation preserves the existing native witnesses.
Passing this monolith is not acceptance of Rules reentry, all native menu
timing, the broader regression suite, or a scheduler repair. The documented158
Rules reentry RGB/state mismatches remain outside this checkpoint; no golden
was refreshed to hide them. Original game behavior is not changed by this
test-only migration.

Evidence in the auditor worktree is under `build/setup-migration-audit-v1`:
`replay-report.json` and137 process logs; `review.diff`;
`constant-change-inventory.json`; `main-attribution/report.json`;
`default-pixel-attribution-recheck.json`; `options-route/protocol-report.json`;
and `final-freeze-recheck.json`. Historical owner failures remain untouched.
