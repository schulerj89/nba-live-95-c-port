# AF4D nested pass return checkpoint

This new bounded slice restores the initializer's saved words at86:AF4D,
the selector's separate saved words at84:E09C, and the human action's saved
B6 at84:E3E6. C stops before84:E3E9 RTL. The native capture additionally
observes that RTL returning to the actual gameplay caller at87:91C3.
All25 naturally captured pass calls match. This implements return-state
restoration, not the skipped initializer body, pass launch or human enabling.

Eleven new files and private output under `build/human-pass-return` are
separate from all thirteen earlier freezes. No existing module/header,
source manifest, session ABI, root checkout, old initializer or enable flag
is changed. There is no commit or push. The C probe compiles only this new
module and probe with the existing type header, using `/W4 /WX`; it links
none of the previous gameplay objects and needs no asset pack.

## Native and C contract

| Source | Contract |
| --- | --- |
| 86:AB2D-AB3B | Save B8/B6/BC/BA/C0/BE/9C/9A at initializer entry. |
| 86:AF4D-AF63 | Restore those eight words in reverse order, preserving all other memory. |
| 86:AF65 | Native RTL to84:E09C. |
| 84:DF7A-DF88 | Earlier selector entry saves its own eight words in the same order. |
| 84:E09C-E0B2 | Restore the selector frame, which differs from the initializer frame in every captured call. |
| 84:E0B4 | Native RTL to84:E2E8. |
| 84:E2E8 | JMP directly to84:E3E6 without memory/register side effects other than PC. |
| 84:E2AC; E3E6-E3E7 | Save then restore the human action's outer B6. |
| 84:E3E9 | Native RTL to87:91C3; the target instruction is not executed by this slice. |

`NbaHumanPassReturnWords` contains the eight source-owned words. `save`
captures a frame from actual caller-entry values; `restore` explicitly
restores the eight fields in source order; `human_tail` restores the outer
B6. `finish` composes the initializer restore, selector restore and human
tail. Frames are distinct caller-entry values, never later snapshots.

This is a typed C return contract. It does not emulate opcodes, a generic
hardware stack or CPU registers. A future C caller must hold these local
saved frames before invoking each body, then restore them on the corresponding
return. The probe supplies independent native caller-entry states to model
that contract; it does not use an after-state or the old whole-initializer
shortcut. Register/physical-stack checks below validate the original native
calling convention separately from the C memory comparison.

## Original source and fresh captures

Original ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen executable:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Fresh Ghidra/recompiler output covers both eight-word save/restore pairs,
the human B6 save/restore and the intervening E2E8 jump. Exact ROM bytes,
tool/source hashes, commands and logs are bound by
`reference-v1/manifest.json`, SHA-256:
`3bcbbda6f2fe4d7a259583920d7d7e9a03854b936bc006813f43223e0d283d04`.
The already frozen human-dispatch reference identifies91BF's JSL84E2AC and
the instruction at its actual91C3 return target.

Accepted left and right captures ran sequentially in fresh isolated Mesen
processes with private executable/settings/home/saves, fixed zero RAM and
ordinary controller input for2,400 court frames. Left uses two released left
taps from fresh `[2,1,1,1,1]`; right retains selection2. The controller route
and directed/neutral B input are unchanged from previous checkpoints.
No WRAM/ROM/PC patch or controlled state injection is used.

The script buffers the actual E2AC prestate when its incoming B bit is set,
and emits that snapshot only if the original owner/gates naturally call
DF7A. The entry is not reconstructed from DF7A. Initializer and return
snapshots are taken at their actual PCs. Sparse raw memory is the exact
13,824 bytes at0000-1FFF and3400-49FF, including the native stack in low WRAM.
Each row also records actual A/X/Y/PS/D/SP/DBR/program bank/PC and a stack
preview independently read through the SNES bus. The preview stops at1FFF;
it neither reads unrelated memory nor zero-fills beyond captured WRAM.

| Route | Events | Calls | Initializer/selector/human/combined | C values |
| --- | ---: | ---: | ---: | ---: |
| left0, selection0-v2 | 178 | 16 | 16 each | 121,024 |
| right2, selection2-v1 | 101 | 9 | 9 each | 68,076 |
| total | 279 | 25 | 25 each | 189,100 |

Every comparison passes. Each C response contains1,891 numeric values:
17 original saved-frame words, all128 DP words,1,408 actor words,160
controller words,128 context words,20 profile-table words,13 order words,
16 globals and a result. Every declared output field/row/vector/type is
checked. The source module writes only its typed saved-word structure;
the probe projects those eight DP words, leaving all other input bytes intact.

Capture manifest hashes:

- `selection0-v2/manifest.json`:
  `5f5faa477492bfcf07aa2e7cf439596a97d1ecbad98d0528421b75b1eccb5e5d`
- `selection2-v1/manifest.json`:
  `b86c52ff3a9fd6a737bc4e61aa8b1948a7e36993b83db5df81e25068eb3fffff`

`native-return-attribution.json` records all25 original frames, their
differences, restored words and register/stack traces. The human's restored
B6 is negative in8 calls and nonnegative in17; no restored outer B6 is zero.
All25 calls execute AB2D. The no-receiver selector-only route is not witnessed.

## Register, stack and memory evidence

All25 calls use these actual SP values:

| Boundary | SP |
| --- | --- |
| E2AC human entry | 1FF9 |
| DF7A selector entry | 1FF4 |
| AB2D initializer entry | 1FE1 |
| AF4D before initializer pops | 1FD1 |
| AF65 before initializer RTL | 1FE1 |
| E09C before selector pops | 1FE4 |
| E0B4 before selector RTL | 1FF4 |
| E2E8/E3E6 human resume/tail | 1FF7 |
| E3E9 before human RTL | 1FF9 |
| 8791C3 actual gameplay return | 1FFC |

The verifier checks every saved stack word against the corresponding earlier
DP value, both16-byte pop depths, the outer2-byte pop, and allthree3-byte RTL
depths/addresses. PLA's final A and N/Z flags are checked from the last popped
word. X/Y/D/DBR/program bank remain unchanged during each pop sequence;
RTL preserves A/X/Y/PS/D/DBR and restores the actual program bank/PC.
The direct jump preserves all registers except PC. Every captured raw byte
outside the exact written DP word set must remain unchanged across each
return segment, including physical stack memory. No pointer is normalized
to a guessed owner or actor.

The supported captured domain is D=0, 16-bit A/X, DP0, DBR7E and a low-WRAM
stack that contains the complete frames. The return instructions themselves
have no decimal-sensitive ADC/SBC; D=0 is the inherited route precondition.
This checkpoint does not generalize to emulation mode, arbitrary DP/DBR,
out-of-WRAM stacks, unknown callers or interrupts during the bounded return.

One left selection crosses court1471 to1472 before AB2D entry. That is original
behavior, preserved in the trace and reported explicitly. All bounded return
segments fromAF4D onward remain in a single court frame. Caller snapshots may
precede that frame; their actual stack contents still match exactly. No clock
is shifted to hide this crossing.

## Integrity, first failures and limits

The verifier uses the strict accepted identity/settings recipe: exact JSON
types and domains; immutable source/tool/ROM hashes; exact process command,
route environment and supported runner limits; every artifact; initial clock
anchors; raw/metadata and independent stack/raw consistency. Private settings,
home and saves are checked before the helper can mutate a copied record.
It rejects missing/reordered frames and verifies the actual nested caller
relationship. All112 mutation cases reject, including17 original saved-word
corruptions, stack/depth/register/flag changes, artifact omissions, clock
shifts and complete C-output corruption. These are implementer-run checks;
independent acceptance is pending.

This probe loads no assets. Its exact successful stderr protocol is empty;
even an asset-loader line or blank line rejects. Its sole argument is the
probe path; stdin contains mode plus current and earlier caller-state paths.
No unnecessary asset dependency or loader diagnostic is synthesized.

The first left native run completed16 calls but failed final isolation
attestation because the new script omitted `observed-script-data-folder.txt`.
That entire failed attempt remains diagnostic-only in `selection0-v1`; it is
not counted in the table. Its initial fixed19-byte stack preview extended
beyond1FFF at high SP. The fresh selection0-v2 fixes both capture issues and
adds the actual87:91C3 return boundary. Its states are not copied from v1.

The first verifier then rejected the original1471-to1472 selection crossing
because it incorrectly required the whole call to stay in one frame. That
failed report/log and verifier are preserved. The corrected verifier limits
the uninterrupted-frame requirement to the return segment. No C/native
mismatch occurred and no C body change was needed. The clean `/W4 /WX`
build, initial successful reports,112-case run and exact sources are frozen.

Probe SHA-256:
`1a5bb2179f6f0e42d2148533b9a6d8fbb7c3439f197a7b653fe189e685f8311e`.
Verifier:
`0bb4af5b5460bfda9300faf3220c7d964c28661b9b237b38a8b1dca5c314f98e`.
Mutation tool:
`f06ba9247a5d7af2aec398cb3e7dec838b97ca97e4d5b90d0745ff239b724115`.
Successful reports are `selection0-initial-v2.json`,
`selection2-initial-v1.json`, `mutations-initial-v1/report.json` and
`native-return-attribution.json`.

The natural AF4D return-state gap is closed through the original human tail.
The87:91C3 instruction itself, later gameplay updates, mode15's subsequent
pass launch, earlier unimplemented initializer branches and full module
wiring remain separate work. This checkpoint never claims normal human play
or a complete initializer from return-only vectors.
