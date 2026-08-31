# Bootstrap checkpoint verifier revision2

The original327-identity checkpoint remains byte-identical. Independent QA
freshly reproduced its bounded C/native results, then demonstrated nine accepted
malformed parsed traces/summaries. The rejected verifier and failed evidence are
retained; passing source replay does not excuse those protocol defects.

This revision changes only diagnostic verification. It does not modify the C
bootstrap, source generator, accepted reused components, ROM, native captures,
clock profile or executable. Full S1 through03DB is still open at CPU80BC,
before the8A57 reset DMA child. Independent acceptance remains pending.

`verify_bootstrap_v2.py` retains the native identity and differential checks.
Its additional `bootstrap_trace_protocol_v2.py` reconstructs the complete
pre-DMA CPU bus clock sequence from the pinned software profile, source bus
addresses,420D writes and refresh rules. No reported C sample/completion time
or captured elapsed total is an input to that reconstruction. It checks exact
read sample/completion clocks and all95048 CPU cycles, including the final
instruction's completion flag. Every opcode/operand fetch must occur once,
in order; JSL's bank fetch may correctly occur after its stack operation.

The same clock progression independently determines all34361 due SPC machine
cycles, including startup catch-up. Every logical deadline, SPC tick and
instruction entry is bound to that sequence. Concrete IPL/0380..038E source
lengths and cycle shapes validate fetches, phase and completion. Only the final
partial instruction is allowed; its PC/phase and ticks must equal the summary.
Refresh totals also derive from this clock progression. Resident and F1 markers
must follow the actual corresponding committed SPC instruction/write.

This is an independent diagnostic translation of the pinned hardware/source
contract, not a second emulator implementation or a claim of physical hardware
clock uniformity. Native SPC callback-master timestamps still describe lazy
catch-up and are not compared to the logical deadlines. Only the original9616
observed native SPC instruction states are native differential coverage;
additional modeled cycles and the partial clear after0387 are source/protocol
checks. DSP private evolution, DSP RAM reads, elapsed DMA/NMI and03DB remain
excluded. There are no fitted waits or new response inputs.

The old nine-case independent corruption tool is reused unchanged. The original
21 local rejection cases and12 additional profile/fetch/marker/width mutations
are also run, each with an accepted baseline. Mutations alter only parsed views
after fresh C execution; raw C output and native files remain unchanged.

Reproduce with fresh output directories, using the frozenv1 probe or a new
private build from its unchanged source:

```powershell
python tools/verify_bootstrap_v2.py --native build/native-bootstrap-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-probe-v3/bootstrap_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-verifier-v2-fresh
python tools/test_bootstrap_protocol_v2.py --native build/native-bootstrap-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-probe-v3/bootstrap_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-protocol-v2-fresh
python tools/test_bootstrap_profile_v2.py --native build/native-bootstrap-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-probe-v3/bootstrap_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-profile-v2-fresh
```

The new report pins both verifier files. The composite freeze directly includes
every original identity plus these new files/reports and the unchanged external
independent tool's identity. The next DMA implementation is separate and is not
covered by this verifier revision.
