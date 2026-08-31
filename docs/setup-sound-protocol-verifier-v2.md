# Sound/SPC verifier protocol repair, separate v2 revisions

These revisions change only verification. All original C modules, headers,
generators, probes, native captures, v1 reports and v1 freezes remain unchanged.
The independently found weaknesses do not demonstrate a C source-state defect:
they show that malformed probe output could pass the old verifier.

## Reproduced failures and repairs

| Component | Old parsed/protocol failures reproduced | New guard |
| --- | --- | --- |
| 65816 sound prefix | Five of the auditor's nine corruptions accepted: non-integer zero statuses, wrong fetch value/address, noncanonical idle address. | Exact integer-zero process status/text shape, ROM byte/address/fetch-slot checks, canonical idle request. |
| 65816 sound initializer | Same five of nine accepted. | Same shared fetch/status guard, retaining width-sensitive initialization. |
| SPC resident | All seven new cases accepted: bool/float zero status, stderr, fetch value/address and idle value/address. | Strict status/stderr plus uploaded-source fetch slots and canonical idles. |
| SPC initializer | Bool/float zero status and stderr accepted; extra JSON stdout already rejected. | Strict status/text/stderr. Its binary trace does not expose fetch/idle requests; no new fetch observation is claimed. |
| SPC F1 control owner | Bool/float status already rejected; extra stdout/stderr accepted. | Strict integer status and exact empty byte stdout/stderr. This effect-only owner has no fetch interface. |

All five new verifier files end in `_v2.py`. Two new small contracts validate
the exposed fetch streams. The original audited behavior and pending hardware
boundaries are unchanged.

`setup_sound_fetch_contract.py` reads the actual canonical ROM opcode and
determines the encoded operand length for the bounded instruction set. Immediate
accumulator/index widths use the instruction-entry P flags. The check requires
exact opcode/operand bytes at exact source addresses and bus slots. It retains
JSL's noncontiguous bank-byte fetch at zero-based slot 5, after its bank push and
idle. Fetches are excluded from native data comparison only after validation;
the old broad address-window exclusion is removed. Every idle must carry zero
address and zero value.

`setup_spc_fetch_contract.py` applies the same principle to resident source
using its verified ROM-to-ARAM mapping. In particular, CBNE's relative operand
is fetched at slot 4 after the port data read and idle, not consecutively with
the first two bytes. Unexpected fetch slots and nonzero idle padding reject.

Neither helper executes an opcode, derives a register result, predicts a clock
or consumes a native port response. These are protocol checks for already
exposed source work. The SPC initializer's narrower binary observation remains
instruction/register/write evidence; its original source-ROM checks and actual
C fetch-acceptance guards are preserved but not relabeled as a full bus trace.

All process results now require `type(returncode) is int` and value zero. Text
probes require string stdout/stderr and no stderr. The silent F1 probe requires
empty byte stdout/stderr. Errors or debugging output no longer disappear behind
a successful exit code.

## Validation and retained evidence

The auditor's unchanged `test_sound_prefix_protocol_audit.py`, SHA256
`dc83a935668b1b6f6cb1fe709b7b030b2750c844811478c015edaf18d05224be`, rejects all nine
corruptions against both new 65816 verifiers. No alteration was made to that
test or its original v1 rejection reports.

`test_setup_spc_protocol_v2.py` runs identical mutations against old and new
SPC verifiers. Its old-verifier rejection records are retained under
`.analysis/spc-{resident,init,control}-v1-protocol-rejection`; the new counterparts
reject every corresponding case. Original native files and actual generated
traces are not edited by these mutation tests.

Seven additional tests exercise accumulator/index width changes, exact JSL and
CBNE fetch positions, wrong opcode bytes/addresses and idle shape. All pass.
Five fresh private MSVC `/W4 /WX` builds and their original local regression
suites also pass:

| Component | Unchanged fresh source replay | Additional protocol/local checks |
| --- | --- | --- |
| Sound prefix | 46 calls; 3,673 instruction states; 2,392 data accesses; full WRAM endpoints. | Auditor 9/9, original 10 negative mutations plus pending-response independence, 9 original unit cases. |
| Sound initializer | 5 calls; 7,055 states; 2,450 data accesses; full WRAM; 1,264 uploaded bytes. | Auditor 9/9, original 10 negative mutations plus pending-response independence, 11 original unit cases. |
| SPC resident | 16 slices; 182 states; 175 attributable accesses and original output/ARAM effects. | 7/7 new protocol cases, 22 original cases. |
| SPC initializer | 192,818 states; 64,394 C writes and full clear ARAM endpoint. | 4/4 protocol cases, 21 original cases. |
| SPC F1 owner | 2 same-clock publications; 70 visible fields and two full ARAM endpoints. | 4/4 protocol cases, 32 original cases. |

All 142 selected fresh source trace/endpoint artifacts compare byte-for-byte
with the old successful replays: 92 prefix, 10 sound-init, 32 SPC resident,
6 SPC-init and 2 F1 output artifacts. The attestation is
`.analysis/sound-protocol-v2-byte-invariance.json`. Verifier/report identities
change; source behavior and generated work do not.

Each component has its own new `.analysis/*-freeze-v2.json`, with direct source,
verifier/helper/test, native/build/report identities. The original freeze and
all of its entries are retained. Current final proof directories are:

- `.analysis/sound-prefix-v2-protocol-final` and `sound-prefix-v2-regression-final`
- `.analysis/sound-init-v2-protocol-final` and `sound-init-v2-regression-final`
- `.analysis/spc-resident-v2-protocol-final` and `spc-resident-v2-regression-final`
- `.analysis/spc-init-v2-protocol-final` and `spc-init-v2-regression-final`
- `.analysis/spc-control-v2-protocol-final` and `spc-control-v2-regression-final`

Reproduction uses the corresponding `_v2.py` verifier and a fresh output
directory with the original component's documented native/ROM arguments.
For the first two components run the unchanged auditor tool with `--verifier`
selecting v2. For the three SPC components use `test_setup_spc_protocol_v2.py`
with `--kind resident`, `init`, or `control`. Fresh build scripts are unchanged.

Independent audit of each new revision remains required. These repairs do not
accept normal initialization, CPU/SPC visibility, timer/DSP progression, whole
transition phase or repeated Rules-entry parity. They do not change the original
game's quirks or any production wiring.
