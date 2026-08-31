# Independent bounded SPC resident audit

Source/component checks PASS; verifier v1 is REJECTED. Original scheduler freeze
`.analysis/spc-resident-freeze-v1.json`, SHA256
`e969c36f31ef35513c4d2726ad21cb82fbbf47672681bd8661548a0ca493af1e`,
and all original native records remain unchanged. No full SPC execution,
normal-state initialization, CPU/SPC phase or production enabling is accepted.

All23 frozen identities were independently rehashed. Exact reviewed source is
under auditor `build/spc-resident-audit-v1/source`; a fresh private `/W4 /WX`
build passes all16 isolated slices,182 instruction/register states and175
attributed data access positions/values. Its source-owned endpoint effects
match. This is not a complete native ARAM endpoint or full bus comparison:
the1,333 raw bus observations include concurrent DSP accesses. Only the declared
port, CALL-stack and uploaded source/table accesses are attributed here.
Native prestates reconstruct from the initial captured ARAM plus preceding
native writes, exclusively for isolated component tests.

The source's24 compiled byte strings independently match the canonical ROM
payload at `00:C687 + SPC_PC - 0380`;23 states have natural witnesses and
$0441 has controlled coverage. The payload hash is
`0559044860666dc3bae509c93a74134d09bf8ccbece26d774876afeec8923fd4`.
The upload-entry, poll-entry and final raw ARAM snapshots all contain those
same1,264 bytes. The pinned five-file Mesen source reference was independently
rehashed at commit `b9fa69ddc6d0a331fb103fdb5eef6904305703c2`. That reference
is explicitly not asserted to be the installed binary's exact revision;
installed-binary native observations provide the differential evidence.

The reviewed source preserves:

- $0441 acknowledgement and $0443 input-clear wait; $0447 idle publication.
- $044A CALL's high/low stack writes, including an8bit SP wrap, before $048B.
- $0453 CBNE's live input read before its relative operand fetch, unchanged
  flags, two preceding NOPs, and timer-service route on mismatch.
- $0456's8bit ASL: command$85 aliases$05 without an added range check.
- $0613's acknowledgement before the channel/DSP work and its input-port
  read before the separate output-latch write.
- $0619's full-byte XCN and $061B's carry/half-carry/overflow arithmetic;
  channel$FF produces DSP address$06, without an inserted channel mask.
- The underlying ARAM write and separate SPC-to-CPU latch on port writes;
  visible input publication changes neither output latches nor ARAM.

These original behaviors are commented in C. They are preserved, not treated
as port bugs to improve. Independent `tools/spc_resident_audit_probe.c` checks
2,304 combinations: all256 channel bytes, commands$05/$85, four initial flag
patterns, and all256 second-read input values. It checks exact flags, wrapped
table reads, acknowledgement order, unchanged input latch, source-owned
writes, pending DSP read and the CBNE mismatch route including SP wrap.
All2,048 DSP-read attempts refuse without changing state or bus. The tested
command$05 path has53 accepted cycles, independently summed from source;
idle publication and input-clear entry have15 and20 cycles. These are bounded
work counts, not CPU/master-clock scheduling constants.

The independent original-ROM inspection also confirms the documented
initializer omission at$08FF. At$03A3..03D3 the two independently set carries
produce$8F/$F7. The first write-before-DEX loop covers143 bytes$0870..08FE;
the following247 full pages begin at$0900. $08FF is therefore untouched.
This is outside the resident component and is a static source finding only:
an all-zero native fixture cannot dynamically witness the omission. It must
not be silently replaced by an inclusive memset. Exact raw bytes, range hash,
source states and the three observed normal-speed/write-enabled snapshots
are retained in `source-recheck.json`.

At$048B and$0622 the timer or DSP data read remains pending after two fetches;
accept refuses, and no DSP store has occurred. The API intentionally does
not implement a CPU port-write visibility adapter. Its caller must already
know when the input is visible. Native input/output snapshots are not allowed
as production initialization. Timer service, remaining command bodies,
uploader/reset, DSP and cross-clock visibility remain outside this component.

The local22-case test passes when given absolute paths. Its documented relative
path form fails because seven mutations use a relative-string membership test
against resolved filenames and never run. Both results are retained under
`local-tests` and `local-tests-absolute`; this is a test-driver defect, not a
game-source divergence. The test correctly reports failure for unreachable cases.

Independent strict protocol testing finds24 accepted invalid cases:

| Area | Reproduced missing guard |
| --- | --- |
| Process | Boolean/float zero return codes and unexpected stderr accepted. |
| Terminal ownership | Missing final stop, extra terminal idle cycle, and premature instruction-end flag accepted. |
| Source bus | Wrong opcode-fetch value/address and noncanonical idle address accepted. |
| Capture metadata | Missing arguments, wrong command route/kind, extra manifest/source/artifact fields, and a script path outside the capture accepted. |
| Hardware precondition | Parsed internal/external speed1 and ARAM writes disabled accepted despite the API's normal-hardware contract. |
| Unique JSON keys | Duplicate manifest schema, native PC and C PC keys silently accepted. |
| Native route metadata | Unknown initializer boundary tag and CPU-port address outside the capture hook ranges accepted. |

The extra terminal idle changes a15-cycle report to16 without changing any
native result, yet the verifier still passes. The missing-stop case passes
with no stop row at all. These undermine the explicit unresolved-cycle contract,
not merely formatting. All original frozen C output remains good in fresh
replays; the failures show what this verifier cannot reject.

Actionable sites in `tools/verify_setup_spc_resident.py`: `read_native` at18
needs exact manifest/source/path/command/settings/domain checks and unique-key
JSON parsing; line88 needs actual integer zero plus captured empty stderr;
the cycle/state loop at89..100 needs mandatory final-stop/phase/completion
ownership and independent source fetch/idle validation. Capture-state hardware
preconditions need validation against the raw state records, not only comments.

The final independent tool is `tools/test_spc_resident_protocol_audit.py`,
SHA256 `015efc47a70b3c62f264fdb797feed4cf030d4a8a442d4c81d79c6413ffcced1`.
Its24-case failures are in `independent-protocol-v2/report.json`; the earlier
19-case tool/report are preserved in `independent-protocol`. Invoke with the
candidate `--verifier`, unchanged `native-spc-resident-v3`, canonical `--rom`,
the private `--exe`, and a new `--output`. Original verifier SHA256 is
`28f0e2bce89628dec8431faa03f0c1f7f32977e81e4f613e64b16add518d71c0`;
C SHA256 is
`3a0bfd71cf8bb7df04e2e86cb5133bb6adc92ff2e235a8a7a4574409e753d29f`.

Acceptance requires a separately frozen verifier revision and independent rerun.
The original failure history is not superseded or erased by source success.
No production source, native fixture, prior freeze, original game quirk or
runtime enable was changed, and no commit/push was performed by this audit.
