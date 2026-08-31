# Codec verifier revision 2

The independent FB46 audit found two observation-protocol gaps, while its
fresh C run still matched all source states, CPU write positions and payloads.
A fabricated negative intrinsic instruction duration could be compensated by
the adjacent 40-clock refresh residue. Reordering the combined instruction/write
JSON lines could also pass because the old verifier separated the two lists.

The original `codec-freeze-v1.json`, its sources, executable and reports remain
unchanged. New `verify_setup_codec_work_v2.py` uses
`setup_codec_trace_contract.py` to require strict mixed-event cycle order, writes
attached to the current instruction, cycle-one/master-zero origin, and each
instruction's actual bounded six/eight-clock bus domain. Recipes have at most
ten CPU cycles, so no 40-clock transfer between adjacent intrinsic intervals
can pass. Refresh/NMI checks remain separate native conservation checks.

The new verifier also pins the three exact capture script/base/runner revisions.
The freeze now includes the previously accepted strict scheduler verifier as
a direct dependency, closing the earlier external-attestation gap. This changes
neither native fixtures nor the FB46 C source or API.

Reproduce in this worktree with fresh output directories:

```powershell
python tools/verify_setup_codec_work_v2.py --native .analysis/native-codec-work-v1 --previous-native .analysis/native-scheduler-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/codec-build-v7/setup_codec_work_probe.exe --output .analysis/codec-proof-new
python tools/test_setup_codec_work_v2.py --native .analysis/native-codec-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/codec-build-v7/setup_codec_work_probe.exe
```

Local validation: all 112814 instruction states/durations, 28218 CPU writes,
16 payloads, 16 Python tests with subcases and 12 C continuation cases pass.
The auditor's six-case mutation tool rejects both original failures and all
four register/cycle/write corruptions. The auditor reruns independently against
the final freeze before acceptance. The additional FB30 work uses the same
new trace guard, without altering the frozen FB46 component.

This is a bounded codec verification improvement. It does not predict NMI
phase, implement audio/SPC state or repair the repeated Rules transition.
