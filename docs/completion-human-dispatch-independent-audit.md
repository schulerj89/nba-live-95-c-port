# Independent bounded human dispatch audit

2026-08-31. **Human freeze v1: FAIL. Do not enable production human dispatch.** The separate accepted 19-file controller checkpoint is unaffected. The frozen ten files, original native captures and ROM were not modified. Root is preparing a separate repair; this report does not accept that pending revision.

## Identity and independent build

Owner: `.analysis/worktrees/completion-controllers`. Auditor evidence: `.analysis/worktrees/completion-auditor/build/human-audit-v1`.

| Artifact | SHA-256 |
|---|---|
| `build/human-dispatch/freeze-v1.json` | `7f5bd7877726d8a5db90fb33ce442cf9ed6125919204ec8a4020c507be1451e7` |
| `human-dispatch-checkpoint.patch` | `39172f2072fc16c73f0b40c4506327d71ea88ec8553dc618a6b0acf8b74abab8` |
| `src/nba_human_dispatch.c` | `dde327a286d8887fd1fbff601607d597b200997414307becf8a2463c8a5acbba` |
| `include/nba_human_dispatch.h` | `4b8d44cdf77237888df35674532f04584bd9dc81abd9f1a1d8f64b5fc788e4fe` |
| `tools/human_dispatch_probe.c` | `460979e6c39328bc91db0b9bca88239b73a8e34d0f2414f9d72200f1b9c9a7d9` |
| `tools/verify_human_dispatch.py` | `3851b326ba408d28c0bf83e75453a4479832c2999d3825bf5a01ba3836ed3a07` |
| Fresh independent probe | `d9ea90f3ebfb93c5f238b78827070404744c13fad008b826520799c6877c3e51` |
| Original USA ROM, 1,572,864 bytes | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |

`source-identities.json` records every frozen file. The patch was applied in a new auditor directory, checked against all ten exact hashes, and its module/probe freshly compiled there. Other objects are the auditor's previously fresh full build of the exact accepted controller checkpoint, linked read-only; no owner output objects were reused or overwritten. `fresh-build.log` and `compile.bat` preserve the commands.

Actual source references are the owner's `build/human-dispatch/reference-v1/human_bank84.txt`, `human_bank85.txt`, `human_bank87.txt`, their generated C files and three raw banks. The reference manifest is `f561028c1144b794bdcfd8d51a7bf2f0f86c7944b67024a5dd04b7c15ada000d`. Full raw banks and these four ranges match the original ROM:

| Range, exclusive end | SHA-256 |
|---|---|
| `$84:E2AC-E2F2` | `dd22353ddb7bf83945d6f4b7f9155ab327ed206d54ea814b98394cab4b6e9460` |
| `$85:A82C-AB17` | `49db1e33163bc56f8dabd6912dca0184898cd7f3952fafcdfe61d1ea36202c60` |
| `$87:9138-9165` | `ee4ce8dd82c6a1076952af6673de5635648c7b3533fe584f2377a61657181648` |
| `$87:91C3-922E` | `3077898a97fab94a1c49ad972768681adae9277942fbab4b2a5a9ae07fc08c2e` |

## Confirmed original carried-X behavior is lost

`src/nba_human_dispatch.c:49` always passes the actor's boost timer to the existing velocity helper. That is wrong on a reachable airborne path. The helper itself need not be changed: other callers supply actor X. This caller must preserve the original referenced controller-relative word.

1. `$87:91D7 LDX $9A` selects the controller record. `$91D9` reads its direction. `$91E8 CMP #$0080` / `$91EB BMI $922A` uses the sign of wrapped **16-bit** `live - $80`; on that branch X is still the controller at the acceleration call.
2. `$85:A82C` saves direct-page scratch words but does not save/replace X. `$A840 LDY $96` selects the actor through Y. `$A84D LDA $000C,Y` reads the actor's complete 16-bit Z word. Nonzero Z branches via `$A852 JMP $AAE8`, bypassing `$A91F LDX $96`.
3. `$AAE8` restores scratch words, including original DP `$C6`, without restoring X. `$AB06 LDA $0072,X` reads **controller record + $72** on this path. The actor's boost is untouched.
4. If that referenced word is zero, `$AB09 BEQ $AB16` makes no store. Otherwise `$AB0B SEC; $AB0C SBC $C6` subtracts the original 16-bit dispatch delta with wrap. `$AB0E BPL $AB13` preserves a result with bit 15 clear; otherwise `$AB10 LDA #0`. `$AB13 STA $0072,X` stores the result. This is a sign clamp, not a borrow clamp: `$0002 - $FFFF` becomes `$0003` and is stored despite borrow.

For a motion stage that reaches A82C, the controller-relative target occurs when `((uint16_t)(live - 0x80) & 0x8000) != 0` **and the full Z word is nonzero**. Normal live zero qualifies; wrapped live `$8080` also qualifies. Z `$0100` must not be treated as zero. On the nonnegative compare path, the caller reloads actor X before A82C, so the actor timer is targeted, including blocked live `$81`. All prior free-throw/recovery/flags/receiver/inbound gates still apply. The acceleration-call observation remains `$85:A82C` even if no actor fields change.

This is confirmed original pointer behavior, not a desired improvement. Preserve and comment it. With controller 4, record `$48EB + $72 = $495D` is beyond the current five-record output vector ending at `$492A`; the sparse raw capture does contain it. A repair should explicitly project the referenced word so that case cannot be silently untested.

## Natural and controlled evidence

The unchanged frozen captures pass the fresh C probe:

| Capture | Manifest SHA-256 | Gate / B / motion calls | Compared values |
|---|---|---:|---:|
| `selection0-v2` | `cb1e4f62cdb2366b6daf5e141d5a150d0c71c34dfd7492e1647e0ec214cb238e` | 8570 / 786 / 786 | 337118 |
| `selection2-v2` | `ec0a83297c6b782ecb61d0e8ccdd8b495d8f847b500d8f5e1bf08a852c4ba516` | 8570 / 857 / 857 | 366796 |

Total 703,914 values and 29 frame-crossing stage calls. Independent artifact/source/settings checks covered all 22,095 capture artifacts; original raw evidence and private process isolation are valid. See `source-and-capture-evidence.json` and the two fresh replay reports. These observations do not prove every branch: **52 natural motion calls carry controller X through the airborne path, but actor and referenced timers are both zero**. `carried-x-native-witnesses.json` preserves those observations.

A separate auditor capture adds only ordinary L button hold for court frames `[260,285)` to the original runner's X jump at `[260,263)`. There is no ROM patch, RAM/register mutation, state injection, or savestate load. Original captures are unchanged. New diagnostic directory: `build/human-audit-v1/native-airborne-lx-v1`; 400 court frames, 4,061 boundaries. Manifest SHA `1ff32ce8bbdcca41a4323dabeecb131b8a6a8bcc66ebccd5f7fbddbf54082406`; actual executed Lua SHA `e3166e66f7fb2d35bcfb9126ba662d4fd5d2e0004d1bde942bbafeeff2365c17`.

The fresh frozen C probe fails **three natural calls**, entries 2844/2889/2914, court 267/272/275. Native actor `$39EB`, controller `$47EB`, live zero, Z 8/15/17, delta 2. At the accelerator and exit, native X is `$47EB`. Native actor timer remains 5; C lowers it to 3. The referenced controller word remains zero in this natural witness. `natural-carried-x-witnesses.json` records the full rows/raw observations, and `native-airborne-lx-verification.json` compares all 58,956 diagnostic values. It is a new diagnostic trajectory, not a relabeled original fixture or proof of full human play.

For nonzero controller-relative memory, `controlled-carried-x.json` records a C-only counterexample based on frozen `raw_02844.bin`: change only actor timer to 5 and controller-relative timer to 7, delta 2. Source requires actor 5 / controller word 5; frozen C produces actor 3 / controller word 7. It is not a native injected execution and does not establish natural reachability of a nonzero referenced controller word.

`tools/test_human_motion_alias.py` adds twelve controlled source contracts for zero/nonzero referenced words, full-width Z, wrapped compare, sign/borrow distinctions, zero delta, and actor-target paths. Frozen C fails eight; four controls pass. Source hash `db505bac90d1d004af6d74a601db4b53be2cf43f7bba7954cc4540f43b7defe6`; results `alias-contracts-v1/report.json`. It changes new C input files only, never native fixtures.

## Verification integrity

`tools/test_human_dispatch_integrity.py` SHA `d5abccf77e2c08b24adefc71c769d9a69a60ef01cc553ab1d605a0b581b335b7` ran 25 independent mutations against actual fresh probe output and original capture input streams. Results: `integrity/report.json`, **12 failures** because invalid provenance was accepted:

- Boolean selection; boolean/negative requested frame count; floating sparse-range address.
- Missing or wrong-ROM command arguments.
- Missing environment; wrong selection and frame environment values.
- Empty settings summary; wrong isolation home; wrong observed script folder (helper overwrote the observation).

Changed persisted-settings hash, final-save hash, omitted boundary artifact and omitted runner source were rejected. Eight C output corruptions were correctly rejected: wrong route/accelerator PC, corrupt last actor/controller/context word, floating route, truncated vector and extra unframed output. Thus comparison shape is meaningful, but capture provenance validation is insufficient. Source inspection additionally found row PC equality could accept float-equal integers and frame/court/register metadata lacked strict domains. Repair should follow the accepted controller verifier's exact types, attested command/environment/runner contracts, and validation on a copy followed by comparison of observations. None of these tests mutate the original capture.

## What is and is not translated

| Boundary | Source contract checked | Deliberate exclusion / limit |
|---|---|---|
| `$87:9138` to `$915D` or `$922E` | Signed actor controller field, selected processed latch; negative skips, nonzero latch skips | Invalid positive controller IDs are a C representation rejection, not a claimed native safety check; publication itself excluded |
| `$84:E2AC` to `$E2E4/$E2EB/$E2F2/$E3E6` | Newly pressed native B `$8000`; nonowner switch; owner free-throw return; receiver sign test; live `$82` movement return; otherwise pass | Stops before pass/switch child; all non-B input explicitly continues at unimplemented `$E2F2` |
| `$87:91C3` to `$922E`, observing `$922A` | **Post-action** raw prestate, movement gates, A82C profile byte and velocity helper, with carried-X defect above | Does not model the action child that precedes entry, behavior dispatch, processed marking at `$9276`, actor commit, or the whole actor dispatcher |

Probe inputs contain only actual entry sparse memory (7,936 bytes), original ROM and mode. Native expected exits are read only by Python, never fed into the C stage. Profile +$42 uses the source descriptor pointer and its byte width. The existing expired-counter cap comparison at `$85:AA1B/AA1D` is preserved and commented; receiver sign and wrapped live compare quirks are also commented. The newly confirmed carried-X effect was not represented/commented and is a required repair.

No production source manifest or normal caller enables these ten new files. Effective human input remains intentionally gated by the accepted controller checkpoint. This audit does not accept full human dispatch, button continuations, input-to-action gameplay, timing, normal-journey parity, or production enabling.
