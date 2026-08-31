# Independent bounded sound-prefix audit

Source/component checks PASS; frozen verifier v1 is REJECTED pending the
concrete protocol repairs below. The immutable scheduler freeze is
`.analysis/sound-prefix-freeze-v1.json`, SHA256
`4362e357fcb75630e43ae0e20f7164e44c847d6c04be9e0883e076c3883cb62d`.
This does not accept production timing, normal sound initialization, an SPC
response model, or Rules reentry parity.

All 31 frozen identities were independently checked. The exact source and
direct verifier closure were copied into auditor `build/sound-prefix-audit-v1/source`.
The private fresh `/W4 /WX` build passed all 46 isolated snapshot/register
differentials: 3,673 instruction/register states, every instruction CPU duration,
2,392 ordered native data-access positions and values, and every complete
128KiB WRAM endpoint. The four previous scheduler/interrupt JSON observations
remain byte-identical. The fresh proof is under `native-proof`.

The original ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Its actual bytes and original recompiler decoder independently confirm all 73
static state PCs, width states and instruction mnemonics; 65 have natural
witnesses and eight do not. The ROM-only generator also reproduced the frozen
program exactly. `source-recheck.json` retains the decoded bytes and range hashes.
The caller at `$80:857A` is `20 37 A1` (`JSR $A137`); those six caller cycles
are outside the component. Entry requires E=0, D=0, DP=0, DB=$80, M=0/X=1.
E and DP are explicit caller preconditions rather than fields of this C API;
the capture verifies DP=0. The width and DB fields are checked by `begin`.

The reviewed source preserves PHP/PLP and real stack accesses through $9FA1,
live $53 busy and $0637 stream gates, $062A/$062B/$0635 branches, live DB from
$5A at $A1AF..A1B2, sign-based event sentinels, all eight channel iterations,
and the $A28C unsigned classification comparison. $A29F word INC wraps and
writes the high byte before the low byte; this observable source behavior is
explicitly commented in C (lines119 and228). The source does not substitute
a fixed channel cost or normalize these effects.

Independent controlled guards passed 138 cases in
`tools/sound_prefix_audit_probe.c`: eight channels, four mirror banks, four
word edge values, plus ten distinct source exits. They verify exact pitch-word
copies, high-first RMW values/addresses, stack and index state, and 193 accepted
channel0 cycles plus 35 per empty channel. Those totals were separately summed
from the literal 52-instruction source path and 13-instruction empty-channel
loop; they are test expectations, not implementation delays. All 40,384 accepted
cycles were also cloned and resumed identically. At every terminal mirror read,
all 256 possible responses were rejected without changing any continuation
byte: 32,768 refusals total. Results are in `controlled/report.json`.

At $AAE6 only the three opcode/operand fetches complete. The pending $2140 READ
is not consumed or charged, even though it is marked as the last instruction
cycle. Forty-four natural cases end there; two end before $A2CE sequencer work.
$AAE6 is an idle-port poll, not the later $0B echo. The source after it writes
$2141/$2142/$2143, publishes $0B to $2140, polls the echo and clears the port;
none of that is implemented or accepted here. The probe receives snapshot and
register input only for explicitly isolated component tests, never production
state. The existing snapshot-driven audio renderer is not a timing oracle.

The nine local integrity tests passed. The ten local parsed-view corruptions
all reject; changing only an observed pending SPC value leaves C trace and WRAM
identical. However, the independent nine-case tool found five accepted malformed
protocols (four are substantive identity/type checks; one is canonical padding):

| Mutation | v1 result | Required repair |
| --- | --- | --- |
| Process return code `False` | Accepted | Require actual integer zero at verifier line219. |
| Process return code `0.0` | Accepted | Same exact-type guard. |
| First opcode-fetch byte XOR1 | Accepted | Independently validate fetch bytes against ROM. |
| First opcode-fetch address +1 | Accepted | Validate opcode/operand fetch address order, not merely PC range. |
| Idle event address changed from0 | Accepted | Require canonical idle padding; no physical bus effect is implied. |

Extra stderr, extra stdout, a boolean report cycle count, and a boolean
instruction-end flag correctly reject. The fetch gap is at `source_events`
lines168..195 and the broad fetch exclusion at line236. Native data callbacks
exclude opcode/operand fetches, so their exclusion needs a separate source check;
it cannot prove the corrupted fetch records correct. This finding does not show
that the frozen C emits bad fetches. It shows that the verifier accepts them.

The unchanged reproducer is `tools/test_sound_prefix_protocol_audit.py`, SHA256
`dc83a935668b1b6f6cb1fe709b7b030b2750c844811478c015edaf18d05224be`.
Run it with `--verifier` pointing to the candidate verifier, `--native` to
scheduler `.analysis/native-sound-prefix-v1`, `--previous` to
`.analysis/native-interrupt-tail-v2`, the canonical `--rom`, the auditor's fresh
`--exe`, and a new `--output` directory. Original failure artifacts are retained
under `independent-protocol`. The frozen verifier SHA256 is
`ea9653a47c390b1fc13976123417741752ff0cd80f89c2ce43798b63e2c7c598`;
the unchanged C is
`51cb4798ea81a12b5d8003f71f8a808cb8eab35a193ddc6bf8578393fbe9eaae`.

No original evidence or production file was edited. Acceptance requires a new
verifier revision and independent rerun, retaining this rejection. Even then,
the unresolved SPC handshake, full sequencer, normal initialization, external
refresh/DMA/NMI scheduling and production phase remain outside this component.
