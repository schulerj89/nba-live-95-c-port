# Independent bounded sound-initialization audit

Source/component checks PASS; verifier v1 is REJECTED for the same independently
reproduced protocol gaps as sound-prefix v1. Original scheduler freeze
`.analysis/sound-init-freeze-v1.json` is SHA256
`53323c0a37c5f20bc41ea615133724460107bb52b600c47e3415844c83b34c4b`.
No source repair or native fixture change is required by these findings.

All 30 frozen identities were rehashed. Exact source and verifier dependencies
are privately copied under auditor `build/sound-init-audit-v1/source`. The fresh
`/W4 /WX` probe matches all five native isolated calls: 7,055 instruction/register
states, all CPU durations, 2,450 accepted data accesses with values/order/cycle
positions, and all complete 128KiB WRAM endpoints. All seven earlier JSON traces
remain byte-identical. The ROM-only generator reproduces all 62 static states;
independent original-ROM decoding confirms their PCs, widths, mnemonics and bytes.
There are 58 naturally witnessed static states, not 62 natural witnesses.

The raw caller records and ROM bytes independently confirm the reset JSL at
$80:814A, the JSL at $82:AD48, and three JSLs at $82:ABF2. Each actual call
has bytes `22 73 9B 80`, the three-byte stack effect, eight CPU cycles and
54 intrinsic master clocks. These caller cycles are outside the component.
No NMI interrupts the captured five slices; interrupted execution is not an
accepted native witness. E=0 and DP=0 are explicit source API preconditions;
DP=0 is also checked in the capture. Entry M/X/D and allowed DB are guarded.

Source $9B73 first preserves the entry status and A through its DB guard;
only $7E/$7F normalize to PB=$80. $9B8A decrements the byte at DP$53, and
$9B8C/$9B8E copy the full word at $33/$34 to $5A/$5B while X is16bit.
$9B93/$9B96/$9B97 clear exactly 452 bytes, descending $07ED through $062A.
The following sentinels, settings and redundant channel7 writes retain source
order. $9BED decrements the byte guard a second time. The first two natural
calls show $00->$FE; the final three show $FF->$FD. The reset word copy is
$0000; later copies are $0082. These are live caller effects, not a hardcoded
Setup bank or boolean lock. The original underflow/nesting, high accumulator
byte, descending writes and DB normalization are explicitly commented in
`nba_setup_sound_init.c` lines237..243 and have not been normalized away.

Independent `tools/sound_init_audit_probe.c` passed 144 controlled combinations
of six banks, six guard values and four accumulator high bytes. It verifies
all452 clear writes in exact order, both byte decrements, full-word copies,
final registers/stack, and every WRAM byte outside the separately validated
stack area. It also checks 4,703 base cycles and source-derived +3/+6 costs
for the $7E/$7F normalization branches. The base is independently summed as
452 clear-loop iterations (4,519 cycles) plus55 other instructions (184 cycles).
All 677,448 accepted cycles clone/resume identically. All 36,864 terminal-read
response attempts refuse without changing the continuation. Evidence is in
`controlled/report.json` and `source-recheck.json`.

The source stops before the first $AACD data read of $80:2140. Its three fetches
are complete, but the READ and instruction completion are pending. No SPC
response is supplied to C. The eleven local integrity tests and ten parsed-view
negative tests pass; changing only the observed pending port value leaves the
C trace and full WRAM unchanged.

The upload provenance was independently checked from actual ROM descriptors:
$00:C683 contains length$04F0/destination$0380, followed by 1,264 payload bytes;
$00:CB77 contains zero length/entry$0380. The natural post-upload ARAM slice
$0380..$086F equals every ROM payload byte. Stream SHA256 is
`63abcf0382deef058ede935b135222bea146b62bf0d77e529aeb3df5195c9f16`;
payload SHA256 is
`0559044860666dc3bae509c93a74134d09bf8ccbece26d774876afeec8923fd4`.
This establishes byte provenance only. It does not establish uploader timing,
SPC execution, initialization from reset, or usable production snapshot state.

The unchanged independent nine-case `test_sound_prefix_protocol_audit.py`
also runs against this verifier. Five malformed cases are accepted: boolean
and float process return codes, wrong first opcode-fetch byte, wrong first
opcode-fetch address, and nonzero canonical idle padding. The last is padding,
not a physical bus access. Four malformed output/schema/diagnostic cases reject.
The process check is verifier line248; `source_events` starts at198 and the
unchecked fetch exclusion is line261. Require an actual integer zero and a
separate original-ROM fetch address/byte/order check, with canonical idle fields.
These are verifier defects, not observed bad output from the frozen C.

Original failures remain in `build/sound-init-audit-v1/independent-protocol`.
Verifier SHA256 is
`4940810748c027df73166963830d84ab21e679c6ef00f4881ce279a36b8260f8`;
C SHA256 is
`9cbe62aa484e87c4f48569e3b09da8b907c71399aa94fea823278d44c813227f`.
The reproducer is identical to the prefix audit (SHA256
`dc83a935668b1b6f6cb1fe709b7b030b2750c844811478c015edaf18d05224be`),
using `native-sound-init-v1`, previous `native-sound-prefix-v1`, the private
init probe, canonical ROM and a new output directory.

Acceptance requires a new verifier revision and independent rerun. Remaining
channel-off iterations, command acknowledgements, full resident execution,
sequence startup/dwell, external scheduling and Rules reentry phase remain
unaccepted. No original raw fixture, production file or frozen source was edited;
no enabling, commit or push was performed.
