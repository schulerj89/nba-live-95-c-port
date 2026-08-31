# Pose checkpoint verifier v2: explicit binary arithmetic domain

This changes only the pose verifier and its mutation tests, in new filenames.
The original eleven-file pose freeze, C module/header, probe binary/objects,
ROM, assets and native captures remain unchanged. The original verifier is
retained because independent review found that it accepted decimal-mode
metadata outside the binary arithmetic domain implemented by C.

The new verifier requires `CPU PS & 0x38 == 0` at every `pose.*` boundary:
16-bit A/X and decimal mode off. AEC3/B832 and the caller use ADC/SBC and do
not clear decimal mode. The original C translation models binary arithmetic;
it is not an implementation of BCD arithmetic. Requiring D=0 throughout the
captured route makes that limit explicit, including the commit boundary.
The commit itself has no decimal-sensitive ADC/SBC, so its guard expresses
the shared route precondition, not a separate commit arithmetic divergence.

No original behavior is silently repaired. Natural pose entries all have
D=0. The new guard rejects unsupported CPU state instead of pretending that
binary and decimal arithmetic agree. All earlier natural coverage and limits
remain exactly those in `human-pass-pose-checkpoint.md`: AF1D pose/attachment
through AF30 commit, stopping before the AF4D stack epilogue. Human play is
still disabled and there is no production manifest or ABI change.

The auditor's unchanged `test_human_pose_protocol_audit.py` is copied into
the new private evidence directory and hash-bound there. Its original
13-case report is preserved: ten output/schema/diagnostic mutations rejected,
while D-bit changes at pose.entry, pose.offset.entry and pose.commit were
accepted. The same unmodified tool rejects all13 against v2. Its arithmetic
example is a controlled source argument, not a naturally observed decimal
gameplay execution. No native bytes were modified to manufacture a witness.

The new local test file retains all73 prior integrity/diagnostic cases and
adds D-bit mutations at allnine pose boundary types. All82 reject. These
implementer-run checks do not replace independent review of the repair.

Both unchanged native captures were replayed through the unchanged probe:

| Route | Native AF1D calls | Compared values |
| --- | ---: | ---: |
| left0 | 16 | 168,096 |
| right2 | 9 | 94,554 |
| total | 25 | 262,650 |

All comparisons pass. Every C stdout and stderr byte matches the original
pose final reports. There are no new native captures or expanded behavior
claims. The complete strict protocol, source/artifact/settings/clock/raw
guards and compared fields are otherwise unchanged; the verifier diff is
one mask/message change and two explanatory comment lines.

Original pose freeze `build/human-pass-pose/freeze-v1.json`:
`ee84e1455cb203a4d25273af22277925cba717ede116a51d53b0218daf4e1323`.
Unchanged probe:
`d6f9b5410790a7403ebd234a674d2b2a588d6bfc6ca3b80fdd08eac44e86e961`.
Original ROM:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
New verifier `tools/verify_human_pass_pose_v2.py`:
`04ee6939b15d9d9757cb8f4a665e33def33bfacc80e724dc3104ff9a8a85237c`.
New local test `tools/test_human_pass_pose_evidence_v2.py`:
`b8d1e5e239ce967b2eef84c0d0b0428949268964e9d3ccd50be9e021d36bee85`.
Unchanged independent test source:
`d40d3aaa871f2bd1dbcc924d6d24729ea5ede608330f33d9583d2dd35d5b06c1`.

The three new source files and private reports are bound in
`build/human-pass-pose-verifier-v2/freeze-v1.json`. Its composite explicitly
reuses the old C/probe/captures with this new verifier. All twelve earlier
freezes, including catch11, are rehashed unchanged before freezing. No old
file or earlier acceptance claim is rewritten, and no commit/push occurs.
