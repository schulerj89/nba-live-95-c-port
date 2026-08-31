# Audited standalone SPC initializer

`nba_setup_spc_init` is committed as a standalone source component. It is not
in the production source manifest and does not change `nba_spc.c`, audio,
the game executable, DSP behavior or native scheduling acceptance.

The source takes live registers/bus state and advances the uploaded resident
initializer through explicit bus cycles. It stops before the external F1
publication or the unresolved DSP data read. The two diagnostic entries cannot
be joined by inserting a captured poststate in normal gameplay.

The original upper RAM-clear omission at $08FF is preserved and commented.
Nonzero source tests prove that $08FF retains $A5; the all-zero native capture
alone cannot establish this quirk. The root source-work document corrects one
description typo to the actual `$03DB OR A,$F3`; C and capture bytes are unchanged.

Accepted scheduler freeze `.analysis/spc-init-freeze-v4.json` has 123 identities,
SHA256 `c6cb2abd708ac8fa892dffbad2da1764550501c09ef964a749fa2e4fba2e1f11`.
The independent v4 report is
`completion-spc-init-control-v4-independent-audit.md`, SHA256
`d16b6fb41dad5d5a605aa69e6429436674fd37f32a6bfc60f875ffc0f30ec1fa`.
It accepts the initializer only; its separate F1 v4 rejection remains visible.

Root integration evidence is `build/spc-init-integration-v4`:

- Fresh two-source `/W4 /WX` build.
- 192,818 native instruction states and 64,394 accepted writes match; full
  ARAM endpoints match. All six binary traces/endpoints remain byte-identical
  to the frozen accepted replay.
- Both 21-case source regression sets pass, including nonzero carry tests.
- Four process-protocol, nineteen evidence and six unchanged independent
  parsed-state corruption cases reject as expected.
- Forty-four scalar-schema/callback relation checks pass.

The v4 verifier checks all source-defined scalar keys and CPU callback
relationships. In particular, the pending DSP callback is at PC $03DD with
unchanged instruction-entry registers and the captured entry-plus-six clock.
Its unresolved response value is not supplied to C. Pinned Mesen schema sources
describe the reference implementation; they are not asserted to identify the
installed executable's build revision.

Reproduce using fresh output directories and local evidence:

```powershell
./tools/build_setup_spc_init_probe.ps1 -OutputDirectory build/spc-init-new
python tools/verify_setup_spc_init_v4.py `
  --native ../completion-scheduler/.analysis/native-spc-init-v1 `
  --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' `
  --exe build/spc-init-new/setup_spc_init_probe.exe `
  --output build/spc-init-new-native
```

The original ROM, captured memory, pack and executable are not published.
