# SPC initialization and F1 evidence repair, revision 3

These are verifier-only revisions. All C modules, headers, probes, original
captures, and v1/v2 freezes remain unchanged. Independent v3 acceptance is
pending. They do not implement normal initialization, upload handshakes, clock
advancement, DSP/timer execution, or Rules entry timing.

The resident audit prompted the same bounded check of later SPC verifiers.
`tools/test_setup_spc_evidence_v3.py` runs identical parsed-view mutations against
old and new verifiers without altering evidence files. Retained v2 results are
`.analysis/spc-init-v2-evidence-rejection/report.json` (18 of 19 invalid cases
accepted) and `.analysis/spc-control-v2-evidence-rejection/report.json` (10 of 11
accepted). Both already rejected the extra initial-settings field. The other
metadata, JSON-duplicate, and initializer endpoint cases needed repair.

Both new verifiers enforce exact capture kind, launch arguments, source paths,
manifest/source/artifact/build object keys, typed initial settings, and
duplicate-free JSON. Existing source/artifact hashes, persisted settings, exact
integer-zero exit status, and empty stderr checks remain active. F1 still
requires empty binary stdout. No original C or native result was adjusted to
make the verifiers pass.

The F1 verifier uses frozen `setup_spc_evidence_contract_v3.py`. The initializer
uses new `setup_spc_evidence_contract_v4.py`: its only extension separates the
runner's command timeout (300) from the Lua script timeout (60), exactly as the
original initializer capture runner records them. The frozen resident helper
and resident v3 freeze are unchanged. These are host capture limits, never
emulated response times.

Initializer state boundaries must show normal internal/external speed and ARAM
writes enabled. Entry, post-control, and DSP-entry registers/cycles must match
their raw instruction records; no native $F0 write may change those hardware
preconditions. State files reject duplicate keys and require full-size ARAM.
The pending source instruction must end at exactly its last entry cycle plus
four accepted cycles for SPC $0384 (`MOV $F1,#$30`) or two for $03DB (`MOV $F3,A`).
The pending hardware write/read itself remains external. Reported completed
instructions must equal instruction entries minus the final pending instruction;
reported writes must equal the actual write records. The output DSP-address
latch must match the native endpoint, including the $03D8 publication of $6C.
This closes an accepted extra-cycle and corrupted-latch gap without introducing
a fixed delay or hardware response.

F1 is an effect-only commit after an external owner advances the write cycle.
It deliberately accepts carried write-disabled hardware state and arbitrary
valid timer/port state; no normal-speed requirement is added to that API or
effect test. Its synthetic staged-input sentinels remain explicitly unobserved
by native Lua state. Original enable-edge and directional-latch behavior stays
unchanged.

Fresh `/W4 /WX` builds are `.analysis/spc-init-v3-build-v1` and
`.analysis/spc-control-v3-build-v1`. Results:

| Gate | Initializer | F1 commit |
| --- | ---: | ---: |
| New evidence mutations rejected | 19/19 | 11/11 |
| Earlier status/stdout/stderr cases rejected | 4/4 | 4/4 |
| Earlier regression cases passed | 21/21 | 32/32 |
| Native component replay | 192818 states, 64394 C writes | 2 commits, 70 visible fields |

Initializer instruction/write/endpoint files (six) and F1 endpoint files (two)
are byte-identical to the v2 baselines; see
`.analysis/spc-init-control-v3-byte-invariance.json`. The initializer regression
uses a new path-resolving copy of the earlier test to keep native mutations
reachable when given relative paths. The original test remains immutable. The
nonzero source-only fixture still preserves $08FF, which the zero-filled native
fixture cannot independently prove.

Reproduction from this worktree, choosing new output directories:

```powershell
& tools/build_setup_spc_init_probe.ps1 -OutputDirectory .analysis/spc-init-v3-build-v1
& tools/build_setup_spc_control_probe.ps1 -OutputDirectory .analysis/spc-control-v3-build-v1
python tools/test_setup_spc_evidence_v3.py --kind init --verifier tools/verify_setup_spc_init_v3.py --native .analysis/native-spc-init-v1 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/spc-init-v3-build-v1/setup_spc_init_probe.exe --output .analysis/spc-init-v3-evidence-final
python tools/test_setup_spc_evidence_v3.py --kind control --verifier tools/verify_setup_spc_control_v3.py --native .analysis/native-spc-control-v1 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/spc-control-v3-build-v1/setup_spc_control_probe.exe --output .analysis/spc-control-v3-evidence-final
```

The analogous regression commands use `tools/test_setup_spc_init_v3.py` and
`tools/test_setup_spc_control.py` with the same verifier/native/ROM/executable
arguments and their `*-v3-regression-final` directories. Prior protocol commands
use `tools/test_setup_spc_protocol_v2.py --kind init|control` and their separate
`*-v3-protocol-final` directories. Reports include helper/verifier/build/native
identities; final v3 freezes include their direct helper and test closure.
