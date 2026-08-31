# Human pass selector and initializer integration

The independently accepted DF7A selector, F1C1 metric, and AB2D prefix through
the first AC50 boundary are copied byte-for-byte from the controller worktree.
The original pass verifier and its rejection remain historical evidence; use
`verify_human_pass_v2.py` for the selector. The initializer has its own accepted
verifier. Neither component is in the production manifest, and human gameplay
remains disabled pending the rest of its caller/action/movement lifecycle.

Source freezes: `human-pass/freeze-v1.json` SHA256
`2e66c85ccfff9353e9588c148dd9aab1079d65c856284af85b29adaaddb7211f`,
`human-pass-verifier-v2/freeze-v1.json`
`57ec2da5ad270b55eea9ec5484f8fd49ad6495c8509e88fcaa70fbd9744702a4`,
and `human-pass-initializer/freeze-v1.json`
`7412276c4ea9b2fec043d81fe1a779e5f0ba76113bb19cbc085ffced15d3ef4d`.
The three independent audit documents are retained alongside this note.

Root freshly builds both source components against its current complete
production objects. `build/pass-integration-v1` passes the original left/right
captures: 25 selections and41 metrics compare42,766 values; the initializer
compares216,466 values over25 calls and125 stages. Both preserve the original
crossing, with identical native expected values. The62 selector and69
initializer local corruption cases reject, as do both unchanged independent
39-case suites. The independent initializer source guard passes609 cases:
576 fine directions, eight cancellation prefixes,24 geometry cases and one
no-receiver case. These controlled checks are not natural reachability claims.

The root probes are SHA256
`4d0aaab2705cee0dfdbac51ee885bed675f1ee2b7899e349933d462f87a15f27`
(selector) and
`f0fb5e0ffba058bdf4eeefb52e5320b19a81ca489ac4d60429b0dddf675ad7a8`
(initializer). They include the independently accepted wrapped F34F correction.
Their fresh production objects also contain the separately pending C39C inbound
layout repair; that target function is outside both new component paths.

Original suffix preference, wrapped score comparisons, negative animation
cancellation locks, full-word descriptor behavior, and coincident-coordinate
sentinels remain as implemented and source-commented. The recorded nine old
initializer discrepancies are not cleared by this checkpoint. AC50 bodies,
later pass animations/attachment, full CPU-state returns and whole-game native
parity remain separate work. Do not call the old initializer after this prefix;
it would execute initialization twice.
