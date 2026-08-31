# Independent pose verifier v2 acceptance

PASS for the bounded composite: original pose freeze
`ee84e1455cb203a4d25273af22277925cba717ede116a51d53b0218daf4e1323`
plus verifier-only freeze
`5090e5e4a74812015040ead0227f05c7231acf47f4e40dbe8c878f6056cf22d3`.
The original rejection remains in `completion-human-pass-pose-independent-audit.md`;
that source/native review and its coverage limits remain applicable.

The exact verifier diff changes the CPU PS mask from `0x30` to `0x38`, its
diagnostic, and two explanatory comments. No C, ABI, native fixture or probe
change accompanies it. The guard executes at every `pose.*` boundary, requiring
16-bit A/X and decimal mode clear. It therefore rejects the three unsupported
D=1 inputs previously accepted by the independent protocol test. This expresses
the binary arithmetic precondition; it does not change original arithmetic or
assert a naturally reachable decimal-mode gameplay bug. AF30 commit itself has
no decimal-sensitive ADC/SBC; its guard maintains the route precondition.

Independent checks rehashed all 101 new composite identities and all 206 original
pose identities. Both unchanged native captures were rerun with the auditor's
fresh `/W4 /WX` C probe, SHA256
`adaebc1299f01dc79547a5fd614c944441cdc4660fbaaa1a2d9e975430dc4c7a`.
All 168,096 left and 94,554 right values passed, totaling 262,650. Every stdout
and stderr byte agrees with the earlier auditor run. The unchanged independent
13-case protocol tool now rejects all 13, and all 82 local integrity mutations
reject. New verifier SHA256:
`04ee6939b15d9d9757cb8f4a665e33def33bfacc80e724dc3104ff9a8a85237c`.
Evidence is retained under `build/pass-pose-audit-v2`; the earlier 889 independent
literal source guards remain under `build/pass-pose-audit-v1`.

Acceptance ends before the AF4D stack epilogue. Natural witnesses remain 25
grounded point0 calls; point1, facing8 and signed-byte extremes have controlled
source checks, not natural witnesses. The three observed routes after AD3D do
not validate the catch child. Normal human play, runtime dispatch integration,
full pass behavior and whole-game parity remain unaccepted. No production
enabling, original-fixture mutation, commit or push was performed by this audit.
