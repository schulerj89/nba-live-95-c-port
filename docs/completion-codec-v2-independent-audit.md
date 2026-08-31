# FB46 verifier v2 independent acceptance

2026-08-31. **PASS for the bounded FB46 source component and repaired verifier.** Original codec v1 failure evidence and `completion-codec-independent-audit.md` remain unchanged. This is not acceptance of production scheduling, forward NMI/refresh phase, audio/SPC work, or Rules reentry timing.

The independently checked owner freeze is `.analysis/worktrees/completion-scheduler/.analysis/codec-freeze-v2.json`, SHA-256 `ac4af7141e7f60447b5e50be85b5c0e69f241378474e9dcce6119149b8be67ad`. All 25 frozen file identities match. Auditor source/build/results are in `completion-auditor/build/codec-audit-v2`.

| Artifact | SHA-256 |
|---|---|
| Repaired verifier `verify_setup_codec_work_v2.py` | `ab9ab77c2cdf2dbbca40e4feeb8429141781f0298a8f2662144d95e0f6713fe0` |
| New shared trace contract | `88fe78079a8b7f8ef101da817bbf3c43119f084c4222ac9dfccc348c87b6fe61` |
| Directly frozen scheduler-verifier dependency | `be33e53c5712b491eeed3e506e233106ac700b5b8996282c252a44ebc268eaea` |
| Unchanged FB46 C module | `4787f419083d6588eaf93b1435d874ffd82b32b9a188838cbf3254599afc1a50` |
| Fresh independent `/W4 /WX` probe | `3bbc8ccf38b631fa1c1b5797e6113eff940758faeeae17f138dad7154c2660de` |
| Unchanged native manifest | `392e653f348441a2e80bb2f8f355b37a284fa34c58c3bf261418ce51dd05b52f` |

The source/API, frozen v1 verifier, original fixture, owner executable and original failure reports are unchanged. A new full probe build in the auditor directory uses only the frozen source, with no shared game objects.

Independent source diff review confirms the repair checks strict mixed-event cycle order, ties each write to its current instruction, requires cycle-one/master-zero origin, and validates each instruction through the final endpoint against its bounded 6/8-clock bus domain and 1–10 CPU-cycle recipe size. These guards prevent the original negative-duration/40-clock compensation and mixed-row reorder attacks without weakening native comparisons. Original state, duration, write and payload comparisons remain intact. Script/base/runner revisions are now pinned to the exact reviewed capture sources, and the previous imported-verifier dependency is directly included in the freeze.

The unchanged independent six-case mutation tool was rerun against fresh probe executions. All six corruptions are rejected: backward intrinsic time, mixed-stream reordering, register corruption, instruction-cycle corruption, write-cycle corruption and write-value corruption. `integrity-v1/report.json` records the exact reached mutations. Its unmodified baseline passes 112,814 instruction/register states and CPU durations, 28,218 ordered write positions and 16 payloads, with all 7,102 previous scheduler events preserved. All 16 owner Python tests and 12 C continuation cases also pass independently.

The additional 16 endpoint/96-field native comparison and source/raw checks from the earlier audit remain valid because the C module, header and probe source are byte-identical. The earlier scope limits still apply: typed native entry registers are diagnostic leaf inputs, not production-generated phase; master residuals are conservation using recorded NMI intervals, not a forward clock model. No production enabling is authorized by this acceptance.
