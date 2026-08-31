# Audited standalone SPC F1 control publication

`nba_setup_spc_control` commits one source-defined F1 hardware write without
advancing time. The original directional CPU ports, staged inputs, timer-edge
history, reset conditions and IPL/storage behavior are preserved. The module
remains outside the production source manifest; normal SPC startup, timer/DSP
advancement, audio and hardware scheduling are still incomplete.

The final accepted verifier is v5. For the captured `$8F` direct-page writes,
SPC PS.P must be clear: setting it addresses $01F1 rather than $00F1. This is
only a native-callback precondition. The standalone hardware commit API has
no CPU PS input and retains its valid write-disabled behavior. The rejected
v4 verifier and its independent counterexamples remain unchanged.

Scheduler freeze `.analysis/spc-control-freeze-v5.json` has 222 identities,
SHA256 `4c9218e7642c4bb44061fd43de73b6fbb2ba23746d6585e5fe87cc37efe42cb3`.
Independent acceptance is `completion-spc-control-v5-acceptance.md`, SHA256
`acd0a0983ff63e8d9c7c2e1ea5f7324f7716254f2c4322d1a531d77c71135327`.

Fresh root evidence in `build/spc-control-integration-v5` passes:

- Two-source `/W4 /WX` compilation.
- Two same-clock native publications, seventy visible fields and two full
  ARAM endpoints. All four C input/output files are byte-identical to v5.
- Thirty-two source regression cases, eleven evidence corruptions and four
  process-protocol corruptions.
- Eight unchanged independent boundary corruptions and both independent
  direct-page counterexamples.
- All 512 PS-value/domain combinations for the two native callback sources.

Hidden staged-input/pending-update fields are not visible in the native Map
serialization. Their evidence remains the independent source tests, not a
claim of captured native coverage. Neither this control commit nor the
separate initializer can substitute a captured state for a normal hardware
owner or invent an unresolved DSP response.

```powershell
./tools/build_setup_spc_control_probe.ps1 -OutputDirectory build/spc-control-new
python tools/verify_setup_spc_control_v5.py `
  --native ../completion-scheduler/.analysis/native-spc-control-v1 `
  --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' `
  --exe build/spc-control-new/setup_spc_control_probe.exe `
  --output build/spc-control-new-native
```

Original ROM and diagnostic binaries remain local. The game and desktop
executable are unchanged by this standalone integration.
