# Continuous cold bootstrap through the first NMI, before OAM

This candidate continues the original ROM-only cold boot through4200 NMI
enable, early9B73/9BED sound initialization, the actual CPU/SPC port poll,
native-mode interrupt entry and the first handler prefix. It stops **before
80:8184 writes002103**. It does not park the CPU while SPC initialization runs,
provide an acknowledgement, start the host license scene, or claim full S1,
03DB, boot readiness, Rules phase parity, or production integration.

The accepted first-fill/reset-table source and all prior freezes are unchanged.
This new component requires its own independent acceptance. The reset-table
acceptance receipt is `../checkpoint-qa-20260831/build/s1-reset-tables-independent-audit.md`
(SHA256 `b2351cc343c7404a3db1c6e7bb9f3963b333d644473c45c3ec7a599330d49615`).
The earlier5772-identity composite remains the prerequisite closure.

## State and source ownership

`NbaBootstrapNmi` contains the existing single `NbaBootstrapFill` owner and new
NMI/automatic-joypad/open-bus state. There remains exactly one CPU, SPC, WRAM,
ARAM, VRAM, DMA continuation and master clock. There is no copy or reset at
80BC,80C0,8145, sound entry, or interrupt entry. WRAM0100..02FF and queue
cursors/budget remain the same bytes as decoder scratch and later bus writes.
Initialization accepts only the canonical ROM and declared software profile.
Public state is inspectable, but mutation is not an initialization contract.

The new source module privately extends the frozen first-fill implementation;
it does not change the frozen module. The concrete CPU generator emits418
instruction/width states, with live bus requests and source control flow.
There is no generic opcode executor or captured instruction playback in the
runtime. Re-generation checks all418 against the pinned decoder and ROM.
406 generated states are observed in this one normal capture. Twelve static
variants remain unobserved:80E2/E6/EA/EB M1X0,8A65/66 M1X0,9B81/82 M1X0,
9BE7/E8 M1X1,AB14/15 M1X0. The terminal8184 M1X0 is observed but unexecuted.

| Original source | Owned continuation |
|---|---|
|80:8145..814A |Actual4200=81 write, REP30 and JSL9B73 |
|80:9B73..9B8E |Caller status/bank preservation, guard53 decrement and33-to5A copy |
|80:9B90..9BB2 |Clear062A..07ED, FF at07CD/CE/CF,7F at062F,0 at062E,80 at062D; actual child9BD5 |
|80:9BD5..9BF1 |Bank/status handling, second byte decrement53, Y7 and JSR A4FC |
|80:A4FC..A508 |Clear073A,Y; FF at0742,Y; original JMP AACD |
|80:AACD/AAD0 |Repeated live2140 read and BNE using the carried SPC-to-CPU latch; zero response is not fabricated |
|Native interrupt |Eight actual bus cycles, dummy next-code read, idle, bank/PC/status stack writes, FFEA/FFEB vector reads |
|00:8156 ->80:815A |Original four-byte JML80815A; native interrupt entry is not a seeded CPU state |
|80:815A..8182 |Register/DP save, DB80, CLI,4210/4211 read, actual zero05CB branch, X/Y save,05CB=FFFF,SEP20/LDA80 |
|80:8184 |First OAM address/priority write remains unexecuted and explicitly unowned |

Both guard decrements are byte read/modify/write work, with live reads and
source flags. The normal guard53 goes00→FF→FE; it is not normalized into a
Boolean. The return address and status are real stack writes. No captured
return-A, latch value, or interrupt PC initializes the continuation. Alternate
handler branch816E, poll fallthroughAAD2, unrelated IRQ domains and RTI are
explicit source stops. Previously preserved SPC08FF clear omission and DMA
software synchronization behavior remain unchanged.

## Hardware/profile boundary

The hardware reference is pinned Mesen source commit
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`. New immutable reference copies are in
`build/bootstrap-nmi-reference-v1`; CPU, memory, PPU and SPC references remain
in the prior source closures. The profile is NTSC, zero RAM, default UI SPC
adjustment+40Hz (32040), normal speed and no randomized PPU power-on. This is
an explicit software configuration, not a universal hardware timing claim or
proof that the captured binary was built from that source commit.

- InternalRegisters4200 catches up automatic reads using the **old** enable
  state. A changed strobe, controller register reset, serial sample or shift
  refuses at the actual unowned effect. No no-buttons reply is inserted.
- Readable NMI flag changes at h2; enabled NMI requests its delay at h6. A
  rising enable with the readable flag set requests two CPU cycles. Counter
  processing happens on real CPU cycles, with the post-DMA interrupt lock.
  Delivery occurs only after a completed source instruction.
- The first interrupt is supported only while the original AACD/AAD0 poll is
  active in native mode. Entry preserves the original stack and flag behavior;
  the vector comes from actual ROM FFEA/FFEB bytes.
- CPU reads update external open bus except register bankA4000..4FFF. CPU
  writes do not update it; DMA reads/writes have their own rule.4210 retains
  readable flag in the reference's h2..5 protected window, supplies revision2
  and open-bus bits4..6.4211 uses its own flag/open-bus mask.
- Automatic joypad state follows the pinned reference's lazy catch-up call
  sites. At8184, its next clock2093952 is already earlier than CPU2094260,
  but the next owning catch-up call has not happened. This matches the native
  public state. It does **not** justify claiming an eager physical controller
  strobe has occurred. Controller data and port-sample registers remain
  unrepresented beyond the explicit stop; zeros in this capture do not supply
  that missing hardware behavior.

All refusals are terminal. A hardware refusal within a CPU cycle is not an
advertised resumable adapter boundary. The normal checkpoint itself ends
between completed source instructions, before8184. SPC DSP hidden evolution
remains unresolved as documented earlier; no DSP read value is fabricated.

## Normal capture and checks

`build/native-bootstrap-nmi-v1` is a fresh isolated hidden normal cold boot,
with no CPU inputs, WRAM/register seeds, save state, ROM patch or carried
native prestate. Its own process exited0. It captures18 boundaries, each with
full raw public state and WRAM/ARAM/VRAM. Manifest SHA256:
`718882aa64827150a5a0c5dd0fb108671b14e9ca1a936f81025e832b678c0195`.
The100000 CPU and100000 SPC instruction observer limits bound capture only.
Normal original reads of uninitialized7E221B..221F appear as Mesen stdout
warnings; the declared zero-RAM profile determines those bytes. Stderr is
empty. The earlier captures, draft probes and draft reports remain retained.

Fresh10-source MSVC /W4 /WX probev5 SHA256:
`cb5e65625dcdc75b3eb34af1d94636b318a3debe0f203663aa7f67532756ab7e`.
`build/bootstrap-nmi-verify-v2/report.json` passes:

| Comparison | Amount |
|---|---:|
|Ordinary CPU instruction entries, every register/clock |69,898 |
|CPU data accesses including interrupt entry |32,717 |
|DMA read/write accesses |131,072 |
|Native-observed SPC instruction entries |24,592 |
|Native-observed SPC writes/I/O |11,708 |
|Entry/F1 public SPC scalar fields |84 |
|WRAM at13 CPU boundaries |1,703,936 bytes |
|New NMI/control/open-bus fields at13 boundaries |156 |
|Entry/F1 ARAM |131,072 bytes |
|Final first-fill VRAM |65,536 bytes |
|Final typed CPU/DMA fields |22 |

The strict guard reconstructs CPU/SPC deadlines, refresh, first DMA, NMI
counter transitions,4210 side effects and all eight interrupt cycles without
using reported clocks as inputs. It checks exact opcode fetches, mixed NMI
ordering, stack/vector bytes, source markers and complete stdout. All18 native
hook PCs and register domains bind to raw snapshots. The control projections
must agree three ways: C output, reconstructed profile, and native state.

Source endpoint: master2094260,CPU226159,SPCticks199950,SPC03CA phase3,
1535 refresh stalls. The first vector entry is master2093826,CPU226093.
Native SPC observers are lazy; at both vector and final CPU snapshots their
last observed ticks are199902. This is **not** a joint final SPC-state match.
The first three IPL instruction entries also remain source-only, as before.

The new isolated hardware contracts pass7,722 assertions over all256 open-bus
bytes, readable flags/protected windows, delay/IRQ-lock behavior, old-state
automatic-read catch-up and explicit unresolved controller operations. These
controlled inputs are labeled component tests, not normal startup seeds.
Protocol regressions preserve the old corruption tools and mutate only parsed
views after reads; exact case receipts accompany the freeze. None is a new
independent audit. Full boot/NMI return/03DB remains open.

## Reproduce in a private directory

From this worktree, with each output directory previously nonexistent:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_bootstrap_nmi_probe.ps1 -OutputDirectory build/review-nmi-probe
python tools/verify_bootstrap_nmi.py --native build/native-bootstrap-nmi-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/review-nmi-probe/bootstrap_nmi_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/review-nmi-verify
python tools/generate_bootstrap_nmi_cpu.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output src/nba_bootstrap_nmi_cpu_program.inc --check
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_bootstrap_nmi_contracts.ps1 -OutputDirectory build/review-nmi-contracts
build/review-nmi-contracts/bootstrap_nmi_contracts.exe
```

Run `tools/test_bootstrap_nmi_protocol.py` and
`tools/test_bootstrap_nmi_boundary.py` with the same six verifier arguments.
The unchanged prior independent nine-case and three-case tools are retained
under `build/bootstrap-audit-tool-v2` and `build/bootstrap-audit-tool-v3` and
accept `--verifier tools/verify_bootstrap_nmi.py`. Native captures require the
root-coordinated emulator slot; ordinary source/protocol verification does not.
