# Setup regression migration to real input

The legacy Setup monolith passes against the accepted controller integration
binary and fully re-extracted candidate pack. This migrates historical C
regressions to the real held/released input contract; it does not establish
whole-game parity, native initial-entry timing, or Rules reentry correctness.
No production source or native fixture changed in this migration.

Fresh defaults are Arcade/12 minutes. The two initial images at162/166 use the
independently attributed defaults from `setup-highlight-independent-audit.md`:
each differs by1116 pixels inside the two changed value cells. The remaining
seven initial image hashes stay unchanged.

Seven Main value images change only the restored source shadow strip at
`y=top+16`:30/33/33/24/19/21/26 pixels respectively. The old executable
reproduces all10 prior hashes. The accepted19-line source span and47 native
canvas comparisons support this bounded change; all10 native glyph hashes
remain unchanged. Evidence is in
`build/main-value-image-attribution-v1/report.json` and
`build/legacy-main-driver-attribution-v2/report.json`. These full-image
baselines remain C regressions, not native navigation-trajectory witnesses.

The former implicit Simulation/3-minute state is now reached with eight real
presses and intervening releases. Cursor image cases use explicit input at
163..177, then Down at179..189; all seven old cursor images are reproduced.
Main reentry preserves edited values only after Start commits them. A second
case now verifies that leaving without committing restores fresh defaults.
The mode-entry check allows the complete Team transition after those inputs.

Rules automatic navigation shifts20 C steps, with compensating input-idle
adjustments for the existing native witness mappings. The native RGB and PPU
expectations remain unchanged. The publication assets145/155 now use their
independently audited NBSPPU3 format and hashes, including native zero writes.
Only VRAM entries gain the fourth publication byte; CGRAM entries remain three
bytes. The manual trace decoder now agrees with that format and retains its
exact end-offset, state and write-count checks.

For Options transition images, an explicit real-button route preserves the
historical A168/Start302 phase. Configuration inputs occur during the C
entrance's accepted-input interval, with releases between all presses. This
reproduces all13 old opening and10 old return image hashes without seeding
configuration or PPU state. It preserves C regression timing only; native
initial-entry timing remains unresolved. Route inputs, BMPs, input traces and
the result are in `build/options-driver-attribution-v1`.

Real Main configuration emits two adjustment sounds. Successful submenu
adjustments must add sound events; a blocked Rules slider must retain exactly
the two Main events. Volume checks read the third event, whose observed gain
is30/$40 versus31/$42. This avoids letting earlier Main sounds satisfy submenu
sound assertions. The stale Season tail guard now corrupts an actual ink row
and compares identical journeys against clean and deliberately altered packs.
Rules row13 requests retain native row12 clamping, with source PC in the test.

`build/setup-monolith-migration-v1.log` through `-v5.log` retain each failure;
`-v6.log` records the passing full monolith. Its executable is
`build/nba95_controller_owner_v2.exe`; the pack is
`build/full-extraction-v1/nba95_assets.pak`, SHA256
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
The full broader suite is being rerun separately. Rules reentry still has158
native RGB/state mismatches; no expected value has been updated to conceal it.

Independent review now accepts the migration within this scope. All23 frozen
files match, and a fresh complete monolith run executes137 processes. A separate
431-frame Options input trace verifies all held/pressed/released values and15
pulses, including A168/Start302. The reviewer freshly reproduced all10 Main
images, checked all old BMP identities and seven exact shadow-row changes, and
decoded the earlier four-way default attribution independently. See
`completion-setup-regression-migration-independent-audit.md`. The exact owner
note reviewed before this acceptance paragraph is retained in
`build/setup-regression-migration-reviewed-note-v1.md`.
