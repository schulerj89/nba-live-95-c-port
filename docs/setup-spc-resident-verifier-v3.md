# Resident component evidence repair, revision 3

This is a verifier-only revision. The original C implementation, probe, native
capture, v1/v2 verifiers, and their freezes remain unchanged. No production
initialization, timer/DSP implementation, clock prediction, or Rules parity is
accepted by this revision. Independent v3 audit is pending.

The independent auditor expanded its resident protocol suite to 24 cases. The
unchanged tool accepts all 24 against v1. Our retained v2 reproduction rejects
the six status/stderr/fetch faults previously repaired and accepts the other 18:
`.analysis/spc-resident-v2-independent-rejection/report.json`. The new rejection
record supersedes neither that result nor the original independent findings.

`tools/verify_setup_spc_resident_v3.py` requires a single final stop. It verifies
that `instruction_end` occurs only on the final accepted cycle of each completed
instruction. At uploaded SPC $048B (`MOV A,$FD`) and $0622 (`MOV $F3,A`), precisely
the two opcode/operand fetch cycles have completed before the external timer or
DSP read. The stop must be phase 2, with unchanged entry registers and no extra
idle, completed instruction, missing stop, or accepted hardware data cycle.
Completed instruction durations still compare to the native instruction entries;
these checks introduce no elapsed-time estimate.

`tools/setup_spc_evidence_contract_v3.py` rejects duplicate JSON keys and non-JSON
numeric constants, checks exact manifest and identity object keys, pins the
capture kind and executable/ROM/script argument sequence, and binds capture
source paths to their actual archived script/runner/settings/executable files.
The runner's initial settings have exact nested keys, types, and values. The
persisted application settings may retain additional application defaults, but
must preserve those owned settings. Existing byte/hash checks remain in place.

All three native state boundaries must show internal/external speed zero and
ARAM writes enabled. Their registers, cycle, and directional latches must agree
with the corresponding captured boundary. No $F0 write may change these hardware
preconditions inside the resident capture. The 120 initialization rows must have
the actual `init.source` tag. CPU port observations must use the original hook
banks $00/$80/$82 and addresses $2140..$2143 within the captured interval. These
are validation preconditions, not implementations of the unresolved hardware.

`tools/test_setup_spc_resident_v3.py` retains the prior 22 cases while resolving
every input path before mutation matching. Thus relative paths containing parent
segments cannot silently make a native mutation unreachable. The original test
and its prior results remain immutable; the auditor's reported relative-path
failure is retained as an original-test reproducibility defect.

Reproduction from the scheduler worktree (all output directories must be new):

```powershell
& tools/build_setup_spc_resident_probe.ps1 -OutputDirectory .analysis/spc-resident-v3-build-v1
$env:PYTHONPATH = (Join-Path (Get-Location) 'tools')
python ../completion-auditor/tools/test_spc_resident_protocol_audit.py --verifier tools/verify_setup_spc_resident_v3.py --native .analysis/native-spc-resident-v3 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/spc-resident-v3-build-v1/setup_spc_resident_probe.exe --output .analysis/spc-resident-v3-protocol-final
python tools/test_setup_spc_resident_v3.py --verifier tools/verify_setup_spc_resident_v3.py --native .analysis/native-spc-resident-v3 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/spc-resident-v3-build-v1/setup_spc_resident_probe.exe --output .analysis/spc-resident-v3-regression-final
```

Fresh `/W4 /WX` build passed; unchanged independent tool rejects 24/24; local
regression passes 22/22, including pending timer/DSP response independence and
the original command/voice wrapping quirks. The source replay remains 182
instruction states, 175 attributed data accesses, and 16 isolated slices.
All 16 trace and 16 endpoint files match the earlier v2 replay byte for byte;
see `.analysis/spc-resident-v3-byte-invariance.json`. Native DSP RAM callbacks
remain explicitly outside the attributed source bus subset. This is not a
complete SPC or normal-journey simulation.
