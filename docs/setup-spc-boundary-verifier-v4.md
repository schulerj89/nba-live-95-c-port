# SPC init/control verifier v4: scalar state and callback boundaries

This is a verifier-only repair. `verify_setup_spc_init_v4.py` and `verify_setup_spc_control_v4.py` retain the previous source replay and evidence-envelope checks and add `setup_spc_state_contract_v4.py`. No C, header, probe, native fixture, v1/v2/v3 file, or production audio file changed. Independent v4 acceptance is pending.

The auditor's original v3 rejection is retained in `completion-auditor/docs/completion-spc-init-control-independent-audit.md`, SHA-256 `906841d778b4b21abc15a868fa41d54153df93c87d3a73c1b0d79de1fad7ae17`. Source behavior passed its separate review. The rejected composites accepted six malformed init endpoint views and eight malformed F1 state views: missing/extra fields, invalid register values, or an endpoint PC/cycle unrelated to the actual callback. Comparing only the shared keys of before/after states was insufficient. Original rejection reports and source freezes remain unchanged.

## Exact serialization schema

The common guard requires exactly 268 SPC scalar keys, or 270 keys for F1 snapshots with `source_pc` and `value`. Keys and integer widths come from the pinned Mesen `Spc::Serialize`, `SpcState`, `SpcTimer`, `Dsp::Serialize`/`DspState`, and `DspVoice::Serialize`/fields, not from the observed fixture's key set. Signed DSP samples retain their signed domains. Timer outputs remain raw bytes, not the low nibble exposed by a separate timer read. Prescalers and their edge-history fields retain their source ranges. Boolean fields must be literal `true`/`false`, integers must be canonical ASCII decimal strings within the source domain, and the clock ratio must be a finite positive decimal value. Integers cannot be replaced by booleans, floats, Unicode digits, leading-zero strings, or `-0`.

The original retained local reference is Mesen commit `b9fa69ddc6d0a331fb103fdb5eef6904305703c2`, under `.analysis/spc-resident-reference-v1`. Additional files were fetched from that exact upstream commit into the new `.analysis/spc-state-schema-reference-v1`: `Dsp.h`, `Dsp.cpp`, `DspTypes.h`, `DspVoice.h`, `DspVoice.cpp`, `Serializer.h`, and `Serializer.cpp`. Its manifest records exact upstream URLs and content hashes. They are included directly in both new freezes. This source revision is not asserted to identify the installed executable's build; the immutable native snapshots independently match the resulting schema.

`Serializer::WriteMapFormat` has no enum branch, so the voice envelope enum is absent. `StreamArray` omits arrays longer than 64 in Map format, explaining the absence of the 128-byte DSP register arrays and 64KiB ARAM, while short signed arrays appear as individually named scalar keys. The original capture already stores ARAM separately. The new guard does not invent keys for hidden staged CPU input or pending-update state.

This is a schema/domain check, not execution or validation of every possible DSP state. DSP values remain external to these C components. No captured scalar is used to create production sound state.

## Source-owned callback relations

For init, the three instruction-entry snapshots already match all seven recorded CPU values at `$0380`, `$0387`, and `$03DB`. The new guard checks the fourth snapshot, `pending_dsp`, as well. `$03DB OR A,$F3` consumes opcode and direct-page operand before its data read; `Spc::Read` calls `IncCycleCount` before exposing the read callback. Therefore the callback has:

- PC `$03DD`, rather than the source instruction PC `$03DB`;
- A/X/Y/SP/PS equal to the `$03DB` instruction-entry values, because the OR has not returned from its read;
- cycle equal to that entry plus six hardware clocks and to the recorded `$F3` read's clock, under this captured normal-speed-zero precondition;
- the same DSP address latch as at `$03DB` entry.

The C prefix still stops earlier, after its two fetches and before accepting the unresolved DSP data cycle. Its source-PC/phase representation remains unchanged. The read callback's response byte is deliberately neither compared to a guessed value nor supplied to C; the existing positive test that changes only that unresolved response still passes.

For F1, `$0384 MOV $F1,#$30` and `$03EC MOV $F1,#$01` consume three source bytes before the observed write callback. Both before/after snapshots must show PC `source_pc+3`, exact source/value metadata, and the same SPC clock. The old comparison continues to reject changes to fields not owned by the F1 commit. The source `Spc::Write` invokes this memory-write callback only inside its `WriteEnabled` branch, so these particular native snapshots additionally require `writeEnabled=true`.

That last rule is **only a native capture precondition**. The C commit API still supports `writeEnabled=false`: underlying ARAM storage is disabled, while F1 port/timer/IPL register effects still occur. The general scalar schema accepts this state, and the existing source-only C tests retain it. No normal-speed or write-enabled restriction was added to the C API.

## Validation and preservation

Fresh private `/W4 /WX` builds are `.analysis/spc-init-build-v4` and `.analysis/spc-control-build-v4`. Native source replay is unchanged: init compares 192,818 instruction states across its two slices, 64,394 accepted source writes, and full ARAM endpoints; F1 compares two same-clock publications, 70 visible hardware fields, and two full ARAM endpoints. The unresolved F1 write in the init prefix remains uncommitted.

| Gate | Init | F1 control |
|---|---:|---:|
| Auditor's unchanged new parsed-state corruptions | 6/6 rejected | 8/8 rejected |
| Existing source/negative regression checks | 21 passed | 32 passed |
| Existing evidence corruptions | 19/19 rejected | 11/11 rejected |
| Existing subprocess protocol corruptions | 4/4 rejected | 4/4 rejected |

The common state-contract suite has 44 passing cases, including all eight actual scalar snapshots, representative missing keys across DSP/voice/timers, signed and unsigned overflow, canonical text failures, all pending CPU register relations, early/late callback clocks, native F1 PC/value association, valid write-disabled state, and the native callback's separate write-enable precondition.

Final reports are `.analysis/spc-{init,control}-v4-{independent,evidence,protocol,regression}-final/report.json` and `.analysis/spc-state-contract-v4-tests-final/report.json`. The unchanged auditor tool is `completion-auditor/tools/test_spc_init_control_boundary_audit.py`, SHA-256 `54f9f2ba0975452c66ecde5930237e2cc6ccf6676651a2e3725f6c91929af38c`.

Both old v3 file sets (50 init identities and 49 control identities) were rehashed. Six init trace/endpoint files and two F1 endpoint files are byte-identical to the prior fresh source replay. The new period helper's 666 frozen identities were also rechecked unchanged. `.analysis/spc-v4-preservation-final.json` records these checks. Development reports from before the final callback guard are retained but are not the final validation reports.

Reproduce from this worktree using unused output names:

```powershell
.\tools\build_setup_spc_init_probe.ps1 -OutputDirectory .analysis/spc-init-build-recheck
.\tools\build_setup_spc_control_probe.ps1 -OutputDirectory .analysis/spc-control-build-recheck
python tools/verify_setup_spc_init_v4.py --native .analysis/native-spc-init-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-init-build-recheck/setup_spc_init_probe.exe --output .analysis/spc-init-proof-recheck
python tools/verify_setup_spc_control_v4.py --native .analysis/native-spc-control-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-control-build-recheck/setup_spc_control_probe.exe --output .analysis/spc-control-proof-recheck
python tools/test_setup_spc_state_contract_v4.py --init .analysis/native-spc-init-v1 --control .analysis/native-spc-control-v1 --output .analysis/spc-state-recheck
```

The unchanged test tools accept `--verifier` pointing to the new v4 verifier, `--native`, `--rom`, `--exe`, and a fresh `--output`; the shared protocol/evidence and auditor tools additionally take `--kind init` or `--kind control`. This packet does not implement DSP/timer advancement, a response schedule, normal SPC initialization, or any production timing repair.
