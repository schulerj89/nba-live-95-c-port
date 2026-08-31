# Actual actor-pass telemetry and period-boundary test attribution

This is a port diagnostic repair, not a change to an original-game bug.
The old scheduler projection derived ten physical actor entries solely from
even simulation ticks. A period-ending update increments that tick and returns
before the physical pass; frozen presentation and restart updates keep the
same tick. Consequently 1,189 frames falsely reported a pass in this run.

The only production changes append a host diagnostic boolean, clear it at each
update before early returns, set it at the already existing physical-pass
counter increment, and serialize that observation. The boolean controls no
gameplay branch. The source native capture records actual `$85:963D` entries;
this repair makes the C trace describe actual C execution. It does not establish
whole-frame native scheduler phase or resolve the outstanding Rules reentry.

Two private CLI observers retain the original and corrected 63,800-frame runs.
Only the scheduler object differs, exactly at frames48224..49412. Every other
serialized byte is identical on all63,800 rows. The independent existing
`actor_update_tick` counter advances31,305 times; every increment agrees with
the new flag and published mask/order/delta. Source gate predictions from
simulation advancement, the period return and prior free-throw state agree on
all rows. This particular run has no free-throw diversion differences.

All841 complete initialization state records remain byte-identical, SHA256
`68164c0eed462138d257f21dc60fc2bdd86699125e1da64cc0acfa8bba09f38f`.
The appended boolean occupies former zero padding in this matched build; this
is not a portable serialization promise. The unchanged gameplay85 three
digests and closure `d26e6deec1fdc18e` pass fresh probe runs. No asset or raw
native witness changes.

The camera test had a related observation error: at the horn completion it
treated a credit as proof that the caller entered95AC. Actual owned-state
observations show `caller_waiting=false` throughout this period presentation.
The new model preserves an existing wait but does not create one at that early
return. It matches all63,581 checked owned wait states; the original failure at
49413 reproduces and eight corrupted camera records reject. The existing
runtime probe now checks both pending/no-pending camera waits through complete
presentation/restart, retaining every prior camera assertion.

The scheduler test retains fixed delta2, exact mask03FF/order0..9 and ordinary
even-tick phase, while accounting for skipped calls. Its exact isolated AST
section passes all63,800 rows and114 possession changes, rejects the original
parity-only trace and rejects seven corruptions. The differential-observer
probe checks actual callback counts and published fields on2,000 updates,
still requires byte-identical observer/no-observer game states, and adds a
pause case with no reported pass. These are host execution contracts.

Once fake frozen passes are removed, adjacent physical-pass samples straddle
the period rebuild. The shot test no longer classifies that cross-period mode
reset as a9D6E/A9D0 release. Current carried-state checks remain active; only
transition classification requires the same period. The exact focused section
passes77 shot starts and76 releases. Full CPU test v4 passes these changes and
then fails the still-historical layout1 helper at frame506. No full-suite pass
is claimed; that helper and later image anchors are separate updates.

The build runner also now supplies the configured CaptureRoot to the existing
intro-sequence test. Its previous worktree-relative default could not locate
the manifest. With the explicit existing directory all303 EA-motion and five
text-phase image comparisons pass. No intro pixels, phase mappings or native
assets change; cold-boot timing and audio remain outside that gate.

Evidence is under `build/camera-period-attribution-v1` and
`build/actor-trace-attribution-v1`. Failed camera-reader v1 is retained: shared
stdout/stderr put five intact CAMERA records after audio prefixes. Revision2
extracts exact complete records and requires precisely frames1..63800 in order;
the later PHYSICS observer uses separate streams. Original camera, magnitude,
shot-classification and layout failures are retained. Frozen review copies
allow later test-oracle work without rewriting this checkpoint. Independent
audit is pending.
