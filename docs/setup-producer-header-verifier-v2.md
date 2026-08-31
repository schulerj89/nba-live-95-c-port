# Producer and header native observation integrity, revision 2

The independent producer audit found five malformed parsed-native views that
the first verifier accepted: DMA read/write master clocks changed to 0/4,
a CPU write master clock changed to zero, a DMA observation PC changed to zero,
two instruction rows reversed and renumbered, and two mixed bus rows reversed
and renumbered. The header verifier accepted the first three. The original
captures, source continuations, generated programs, v1 verifiers, tests and
freeze records remain unchanged. The failed audit reports are retained in the
auditor worktree; no failing capture was repaired or replaced.

New `verify_setup_producer_work_v2.py` and
`verify_setup_header_work_v2.py` use `setup_native_trace_contract.py` to check
the native observation protocol before composing source fragments:

- Each original instruction sequence must increase in both CPU and master
  clocks. Mixed bus observations may share clocks but may never go backward.
- Scope boundaries must increase in both clocks. The first instruction must
  match the entry clocks, PC and registers. Every observed instruction and bus
  access must lie within the corresponding first scope.
- Every bus observation must have a matching source PC and both clocks inside
  that source instruction's observed interval. The producer composes its
  residual instructions with the five raw codec instruction sequences only
  after checking original chronology; it checks codec writes as well as the
  residual mixed bus stream against that full sequence.

Mesen reports some final CPU data accesses at the same clock as the next
instruction's entry. Association therefore permits either adjacent matching
PC at an inclusive endpoint. Induced WRAM observations can share both clocks.
DMA observations belong to the instruction after the `$420B` trigger; their
CPU is stalled there while master clocks progress. These native conventions
are preserved. Existing separate positive checks still validate NMI hardware
stack writes, caller JSL writes and induced WRAM callbacks before exclusions.
The observed `$80:815A` NMI-entry-to-resume gap is retained, including the
header observer's RTI read convention. This is not a full native CPU-read or
interrupt-body opcode gate.

The unchanged independent five-case mutation tools now reject all five cases
for each verifier. Ten new focused protocol tests cover valid inclusive
endpoints and same-clock effects, both clock domains, wrong source PC and
clock-pair association, source entry and scope bounds. A valid synthetic DMA
interval can grow without rejection: this guard does not impose a measured,
fitted or fixed DMA duration. The frozen thirteen Python contract tests run
against each new verifier through small v2 wrappers, including each probe's
eleven C continuation contract cases. Both existing nine-case source/payload
mutation suites also pass against the new verifiers.

Fresh probes are retained in `.analysis/producer-build-v10` and
`.analysis/header-build-v3`. Full source-work proofs from these builds are in
`.analysis/producer-mutations-v3/baseline` and
`.analysis/header-mutations-v3/baseline`. The producer still matches
155750 instruction states, 40003 CPU write positions, 27726 DMA bytes and all
four 550560-CPU intervals including its caller JSL. The header still matches
126 instruction states, 66 CPU write positions, 8206 DMA bytes and all four
440-CPU intervals. No source work, original quirk or production timing changed.

From the scheduler worktree, use fresh output directories:

```powershell
python tools/test_setup_native_trace_contract.py
python tools/verify_setup_producer_work_v2.py --native .analysis/native-producer-work-v1 --previous-native .analysis/native-codec-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/producer-build-v10/setup_producer_work_probe.exe --output .analysis/producer-proof-new
python tools/verify_setup_header_work_v2.py --native .analysis/native-header-work-v1 --previous-native .analysis/native-producer-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/header-build-v3/setup_header_work_probe.exe --output .analysis/header-proof-new
python tools/test_setup_producer_work_v2.py --native .analysis/native-producer-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/producer-build-v10/setup_producer_work_probe.exe
python tools/test_setup_header_work_v2.py --native .analysis/native-header-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/header-build-v3/setup_header_work_probe.exe
```

Independent acceptance of these revisions remains required. They do not
predict DMA service/alignment, refresh, interrupt/controller/audio work or
SPC execution. They do not establish carried phase, wait epochs, repeated
Rules brightness/RGB parity or Custom-return hidden VRAM parity. Earlier
source documentation's integrity claims are superseded by this explicit v2
native protocol scope; earlier frozen reports remain historical evidence.
