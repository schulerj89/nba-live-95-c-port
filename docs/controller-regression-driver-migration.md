# Regression drivers after canonical controller integration

Full suitev7 first failed `gameplay65_flow_probe` at code21: its Player Setup
fixture expected one Left press to jump from right to left. The accepted
native handler at `$81:A7D0-$A843` changes one position per press. The updated
fixture now checks right2 -> neutral1 -> left0 and the reverse route, including
release updates between presses. It checks canonical selection at each stop
and the legacy side only when assigned. It does not alter production behavior.

The fresh probe passes three team pairs, five presentation phases and30 lineup
cards in `build/gameplay65-controller-migration-v1.log`. Full suitev8 then
passes this gate and reaches the core extraction test after the entire Setup
monolith. That extraction test needed the existing read-only capture root.

The core debug-screen fixture also assumed implicit Simulation/3-minute
defaults. It now uses real released inputs to reach Simulation/5 minutes at
the same C step170. Both historical complete BMP hashes are unchanged, as are
the expected telemetry and sample count. The original primary executable
reproduces those same hashes; the new executable with the explicit input
script matches byte for byte. Commands, BMPs, logs and hash comparison are in
`build/debug-telemetry-driver-v1/report.json`; the revised debug check passes
separately in `build/core-debug-migration-v1`.

These are C regression-driver corrections supported by the already accepted
native input/configuration contracts. They do not establish native entrance
timing, production human play or full-game parity. No native fixture or
production source changed. Full-suitev7/v8 failures remain recorded; v9 is
running with the explicit capture root and the corrected drivers.
