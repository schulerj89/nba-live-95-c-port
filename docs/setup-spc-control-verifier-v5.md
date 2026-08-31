# Native F1 callback verifier v5

This is a verifier-only repair. The source C hardware commit API, resident/init
code, native captures, and v4 packets remain byte-identical. The accepted initv4
packet is unchanged. The independently rejected controlv4 evidence is retained.

The added guard is source-address ownership, not a new emulated hardware rule.
Pinned Mesen commitb9fa69ddc6d0a331fb103fdb5eef6904305703c2 maps opcode8F to
Addr_DirImm/MOV_Imm (Spc.Instructions.cpp207,369-375,739-746).
GetDirectAddress (Spc.cpp567-570) adds0100 when SpcFlags::DirectPage is set;
SpcTypes.h44 defines that bit as20. MOV_Imm writes the resolved address without
changing PS. Thus original resident8F30F1 at0384 and8F01F1 at03EC can generate
the captured00F1 callbacks only with PS.P=0. With P=1 their destination is01F1.
The before/after callback snapshots both require that flag clear.

New setup_spc_control_contract_v5.py first applies every existing v4 schema,
numeric-domain, source-PC/value, consumed-PC and native write-enable check,
then rejects PS&20. New verify_setup_spc_control_v5.py uses that guard. It does
not weaken the standalone F1 commit API, which has no CPU PS argument and must
still accept write-disabled hardware commits. No F1/RNG/CPU/SPC timing or normal
initialization behavior was changed.

Fresh /W4/WX build `.analysis/spc-control-build-v5` passes the two same-clock
native publications,70visible fields and fullARAM endpoints. Its four binary
input/output files are byte-identical to the v4 baseline. Auditor's unchanged
2direct-page corruptions now reject; previous8independent boundary corruptions,
32C/regression cases,11evidence corruptions and4protocol corruptions pass.
A separate512case source-domain test checks all256PSvalues for both callbacks:
only the P bit is newly restricted. Previous106controlv4,123initv4 and1013rolev2
identities were rehashed unchanged in the preservation report.

Independent v5 acceptance is pending. This repair does not establish a normal
SPC startup, timer/DSP continuation, acknowledgement schedule or phase predictor.

Reproduce with a fresh directory:

```powershell
& tools/build_setup_spc_control_probe.ps1 -OutputDirectory .analysis/spc-control-v5-rebuild
python ../completion-auditor/tools/test_spc_control_direct_page_audit.py --verifier tools/verify_setup_spc_control_v5.py --native .analysis/native-spc-control-v1 --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --exe .analysis/spc-control-v5-rebuild/setup_spc_control_probe.exe --output .analysis/spc-control-v5-recheck
python tools/test_setup_spc_control_contract_v5.py --native .analysis/native-spc-control-v1 --reference .analysis/spc-resident-reference-v1 --output .analysis/spc-control-v5-domain-recheck
```
