# Action checkpoint diagnostic protocol repair

This is a verifier-only repair. The original action11 freeze, all eleven source
files, native captures, C module/probe, executable and objects remain unchanged.
The following aligned11 freeze is unchanged too. No production source manifest,
human routing, or game behavior changes here.

The independent auditor found that `verify_human_pass_action.py` accepted three
invalid stderr views while stdout still matched: extra `ERROR` output, the
missing asset-loader success line, and a forged different-pack line. The
original independent ten-case report and its unmodified test source are copied
under `build/human-pass-action-verifier-v2`. The original verifier accepted
three of ten mutations; the original reports are preserved.

The new `tools/verify_human_pass_action_v2.py` requires exactly the one loader
success line emitted by the unchanged C probe. It includes the actual asset
argument, actual file size, and 263 assets belonging to the already SHA-pinned
immutable pack. Missing, extra, duplicated or altered stderr fails even when
all C result words match. The process exit code must also be an actual integer
zero. Native ROM/Mesen/script/environment/raw/clock checks and the full typed
stdout comparison remain unchanged.

The original two captures still pass: 25 AC50 gates, five complete B00B children
and seven B47A calls, comparing 64,269 values. On each route, both captured C
stdout and stderr are byte-identical to the original frozen final outputs.
The original C/probe/native fixtures have not been regenerated or corrected.

`tools/test_human_pass_action_evidence_v2.py` rejects 55 mutations. Its stdout
mutations now carry the valid baseline stderr, so they cannot accidentally pass
because a different diagnostic guard rejected them first. Added cases change
the diagnostic path, byte count, asset count, newline, duplication, error text,
or process result type. The byte-identical independently authored ten-case
protocol tool rejects all ten when run by the implementer against v2. This is
not a claim of independent v2 acceptance; the auditor must review the new freeze.

Verifier SHA-256:
`b03c72c2e2059f5e0ada621ead4a8ca3cb4cb8578d0b72b7e22a99369d31539a`.
Expanded mutation tool SHA-256:
`514e0f402296f19b9d98e2cdfd48035c767868c41de9d659c4935796dee0fdc6`.
The unchanged action probe is SHA-256
`1d201f51404ddb613a6cdb8c3cf7876d357869fef2179d5e589a965fdbb3107d`.

The new three-file patch and freeze are under
`build/human-pass-action-verifier-v2`. They bind the preserved independent
counterexamples, all test outputs and original checkpoint identities. The
action checkpoint still stops before AD0E/ACA9/AFC4/AF1D continuations and does
not enable normal human play.
