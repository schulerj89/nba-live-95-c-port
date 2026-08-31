# SPC initializer v4 accepted; F1 verifier v4 still rejected

The initializer v4 composite passes its bounded independent review. F1 control
source remains accepted as previously reviewed, but its v4 verifier still
accepts an impossible direct-page callback state. No original C, native fixture
or earlier rejection was changed. This is not normal SPC startup, DSP/timer
advancement, production audio, or phase acceptance.

All 123 identities in scheduler `spc-init-freeze-v4.json` and all 106 in
`spc-control-freeze-v4.json` were independently rehashed. Their SHA-256 values:

- init: `c6cb2abd708ac8fa892dffbad2da1764550501c09ef964a749fa2e4fba2e1f11`
- control: `0de81cb230b32f96eee417d98829e4bdbe187b33adc042bb1f33ad89c6500c2c`

Fresh private copies compiled each original two-source probe with `/W4 /WX`.
Initializer replay passes 192,818 instruction states, 64,394 accepted writes,
and its full ARAM endpoint. The control prefix still stops before F1 commit;
the RAM-clear prefix stops before accepting the pending DSP data read.
Control replay passes two same-clock publications, seventy visible fields and
two full ARAM endpoints. All eight source trace/endpoint artifacts remain
byte-identical to the auditor's v3 runs.

All original independent fourteen malformed-state cases now reject: six init,
eight control. Existing regression checks pass 21/32, evidence negatives
19/11, and process-protocol negatives 4/4. The common state suite passes 44.

## Schema and original behavior

The new 268-key SPC / 270-key F1 schema was checked against the pinned Mesen
Spc serialization/types/timers and additional DSP, voice and Serializer files
at commit `b9fa69ddc6d0a331fb103fdb5eef6904305703c2`. Integer/signed sample
domains, short arrays, booleans and field names match that source. Map output
omits enums and arrays longer than 64; the guard correctly does not invent
voice envelope-mode, large DSP-register or hidden staged-input keys. Mesen's
Map integer representation is signed int64; this bounded capture contract
does not accept a negative/overflowed clock. The pinned reference revision is
not claimed to identify the installed executable build.

Pending initializer DSP metadata is now tied to the original OR instruction:
callback PC=$03DD, unchanged A/X/Y/SP/PS from $03DB entry, actual read address
$F3 and clock entry+6 under the captured normal-speed condition. Its DSP latch
is tied to the entry latch. The unresolved read value is not supplied to C.

The already reviewed source quirks remain unchanged: the RAM clear omits
$08FF; F1 preserves directional output/staged-input distinctions, timer edge
history and rising-edge reset behavior. Native Map omits staged inputs and
pending-update state; synthetic source checks remain their evidence. C's
write-disabled F1 commit stays valid even though these actual native write
callbacks require WriteEnabled=true. See the original source audit for the
nonzero $08FF witnesses and 8,192 control combinations; none was discarded.

## Remaining F1 callback defect

`setup_spc_state_contract_v4.control_boundary` checks source PC/value, consumed
PC and WriteEnabled but fails to constrain SPC **PS.P ($20)**. These captured
instructions are `8F 30 F1` at $0384 and `8F 01 F1` at $03EC.

The pinned source is unambiguous: `Spc.Instructions.cpp:207` dispatches opcode
$8F through Addr_DirImm; lines 369–375 call GetDirectAddress on the direct-page
operand. `Spc.cpp:567–570` adds $100 when PS.P is set. MOV_Imm at lines 739–746
does not change PS between address formation and the write callback. Therefore
PS.P=1 writes **$01F1**, not $00F1, and cannot produce the capture script's
callback filtered to address $F1. This is an SPC addressing condition, not the
65816 decimal flag.

The independent tool `test_spc_control_direct_page_audit.py` changes both
before/after PS strings to PS|$20 for each publication separately. Both full
v4 replays still pass all seventy fields and endpoints. Original files, hashes
and C inputs remain unchanged. The report is retained at
`build/spc-control-audit-v4/independent-directpage-v1/report.json`.

The repair belongs only in the native callback gate: require PS.P clear for
these two source instructions. The F1 commit C API has no CPU PS input and
must not gain an artificial restriction. No natural P1 callback is claimed;
the test proves the verifier currently admits source-incompatible metadata.

| Object | SHA-256 |
| --- | --- |
| init v4 verifier | `9fa6f5ccabc2f093c64b9b96e3bee0db27151228e1686012b56e8d59a54eca03` |
| control v4 verifier | `ddcb1ece550cbda9d58639cca15ba4cfc1e7e992fb24b3c397dc171b0824a00c` |
| shared v4 state contract | `ee5deb33c16c4da28c2fec05f8088634714e8340f236218658fe34db6ad53838` |
| independent direct-page tool | `a120563cbed1612bcdf8e07373cef869eb127ed075914a5df1ed27a7890db22a` |

Evidence is under `build/spc-{init,control}-audit-v4`; exact old/new output
preservation is in `build/spc-v4-independent-preservation.json`. Original v3
rejection/source review remains in
`docs/completion-spc-init-control-independent-audit.md` unchanged.
