# Independent bounded HUD repair acceptance

Accepted on 2026-08-31 for the implemented ordinary score-panel, clock, expiration, pause refresh and return-reset behavior. This is not complete HUD, native CPU/NMI/scanout timing, or normal human-gameplay acceptance. The final verifier's general mutation certification remains pending; this review establishes the frozen known-capture and source contract below.

## Exact packet and independent execution

Controller packet: `C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/worktrees/completion-controllers/build/hud-repair-v1/freeze-final-v1.json`, SHA-256 `60f9d3157369b0dfb19068424c0099ec6655ad1b0a34e27fd987c061755249c8`. Independently read and hashed all **13,697 identities / 2,430,971,039 bytes**, with zero missing or changed objects. That inventory retains the earlier attempts, raw captures and the unqualified comparison failure.

Reviewed production identities:

| File | SHA-256 |
|---|---|
| `src/nba_gameplay_hud.c` | `932d41afebb094be5e84ab3cedb8adbbb2bd540eb72e160354c9aa2e762e25a6` |
| `src/nba_tipoff.c` | `055d45b91b086fa4961762a588891ff35cca7034c96820b5a4dcf7705402c4aa` |
| `include/nba_gameplay_hud.h` | `c24fc810fff402a0676e5d78701a7220851209c8ac46df0ce3469e88a2990298` |
| `include/nba_tipoff.h` | `3cd135915a82f492559c1a19453588b4efc2d4897fc6da38217b463586a10fa5` |
| `include/nba_assets.h` | `9d5b21e89eb70146613c5b433ae42ef282c365a0455afd200418babbac247c39` |

The fresh author builds `native-probe-build-v7`, `runtime-build-v5` and `contract-build-v2` bind respectively 9, 40 and 40 compiled source files. Independently checked their current source identities and frozen executable/header/build artifacts. This audit did not independently recompile these binaries; root will compile the integrated sources separately. The native executable is `75fd67691b5a707d4586c98d80f2bf1fb511c4af302d7af13ca67e8796e82952`; the contract executable is `e4d9f836aaf7aef24a8a84019d40b3e484056d7e27c4cf4bcc76c11ae7c287ea`.

Independently reran the final known-capture verifier into `completion-auditor/build/hud-final-independent-v1` using the command in HANDOFF.json with a new output path and explicit `--bounded-shared-clock-read`. It returned the same 746 bounded pairs and two retained clock-read dependencies, with `full_atomic_parent_pass=false`. The final verifier source is `6f648b0e5b8fbcf24d24da4c010cb08c92872f9005d3550e9b9c4bc0c6c13fb0`.

Also reran the frozen contract executable successfully: all 65,536 timer words, 655,360 shot/game-clock combinations, 42 malformed map cases, legacy-resource production refusal without changing the object, reset preservation, and three clear boundaries. Its printed initialization refusal is the intentional legacy-pack negative case.

## Original source and actual callers

Used the actual original ROM, SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`, raw capture data, existing listings, and direct decoding at verified instruction boundaries. Source review was not approval based solely on the implementer's report.

| Source boundary | Reviewed behavior |
|---|---|
| `85:EDAC–EDB6` | Only positive signed 08DE decrements. Zero and negative values stay unchanged. The old unconditional host decrement was a port defect. |
| `83:CC10–CC7A` | Separate dispatch decrement, zero-to-FFFF expiration, sequence increment before child call, and actual long-table dispatch. All 44 table pointers match the ROM. Unsupported children retain their real pending PC rather than pretending to return. |
| `87:94A5`, `87:BBE9–BD2E` | Canonical snapshot 092A remains distinct from current clock 0928. Original first/later 3600/3601 thresholds, mirror arithmetic, and final nonzero tenth are retained. Expiry writes remain in the existing canonical event owners. |
| `83:EBDB` | Original clear ranges retain the small clock below 3,600 ticks; exactly 3,600 takes the full score-panel clear. The seven simple clear-table entries match original bytes. Complex foul clear `83:EC60` remains unresolved. |
| `87:BA5E`, `87:9578–9580` | Wrapped CC10 sign gate, positive shot/game-clock early return, original increment-before-subtract endpoints, and separate 09C6 reset notification/BACB clear. All 11 tilemaps match original ROM bytes. |
| `83:CE36–CFD5` | Real category/assist inputs, source branch order, shared 07F6 rejection draws, phase/kind/counter effects and preserved fields. All three captured selections match, including RNG 9587→38348 with one rejected draw. |
| `86:9DFB/A5AA/A9F7`, `85:A1BC–A1C7` | Shot category stores and nonnegative scoring-context assist store feed the selector. Temporary classifier save/restore is not invented as a lasting producer. |
| `86:8352/835D`, `86:84DB/84DF`, `86:858E/8592` | Pause clears canonical 08E2 before requesting the current panel. Typed pause/timeout return projects B99A then BA54, retaining working map/CHR, published CHR/mask, clock text, phase, category, assist and counters. A full HUD memset would be wrong here. |

Independently exercised original BA5E opcodes for 327,680 called-domain cases across ten game-clock edges, covering the full 16-bit shot-word space through CC10's wrapped gate. This confirmed frame 2 at raw 60, frame 10 at 599, negative/zero selection 0, and the early-return cases. It is controlled source proof, not a natural BA5E witness or a second C execution claim. The separate C contract run above covers 655,360 complete dispatch cases. These original endpoints are explicitly commented, not normalized into conventional rounding.

The adapter reads current canonical home/right and visitor/left teams and scores. The renderer replaces only the indexed HUD projection over copied gameplay VRAM/CGRAM; it does not recreate a scoreboard as RGB artwork. Shared timer, sequence, kind, event bits and RNG return to their preexisting owners, rather than new independent shadows.

## Display and state evidence

The old match clock already moved from 43,200 to 36,796 in 7,200 frames; the captured WEST 2 / ORLANDO 0 / 11:49 BG3 image was stale. The candidate's runtime uses ordinary input and actual Tipoff init/update/render, with no score/clock/actor/RNG seeds. This is the existing direct-scene route, not full cold boot.

Independently inspected the current runtime-v5 images: CHICAGO/ORLANDO current scores, later 2–2, pause 2–4, and small final-minute clock 59.8 then 59.4. First-panel images show 11:32 then 11:31 and subsequent clear. The legacy filename `second-basket-pending.bmp` now contains a completed ordinary later score panel.

Parsed runtime-v5 independently: all 121 rows from outer ticks 4500 through 4620 are identical after excluding the outer tick. Both pause images have SHA-256 `2d95f5113029289090d51a739437adb09652a3b12e7d8f305f9d9c03eecef94b`. At return tick 4684, timer/sequence and clock mirror/frame become FFFF while clock, phase and old clock text remain. The runtime ends at clock 3569 with the two final-minute views; its existing counters report 120 pause holds, 182 formation holds, 7,630 dead-ball clock advances, 1,942 dead-ball holds, and 32,001 live clock steps.

Native lifecycle-v3 is pinned by manifest `ac0da740300c8afc39d76e7620319cbb34b79420fae23927826b30dc29c0aa74`. It has three natural requests selecting kinds 1, 6 and 1. The 746 comparisons use each component's raw before-state, not its after-state as C input. Working map, generated CHR/text and the bounded scalar projections match except the stated two 08F6 observations. Native BA5E calls are absent from this capture; its coverage is explicitly source/controlled only.

## Assets and retained limitations

Pack `f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09` contains 264 resources, 89,442,736 bytes. Independently compared all 263 earlier payloads/metadata unchanged and all nine old HUD sections unchanged. New resource 286 is 3,926 bytes, `4a39e5d5464b676eed999823c178e41f3251a1a830b96221cfd3d3a50c1c0f2d`; its eleven added maps are exact original bytes. The initial 63 CHR tiles remain native-decompressor-attested: the retained standalone format-30 attempt did not establish an independent decoder. Production requires complete v2 resources before mutation. Root's already integrated portable extraction/upgrade pipeline should be retained, not overwritten by the packet's older extractor copy.

The following remain outside this acceptance:

- **Full atomic native parent parity.** At outer CC10 and D1FD, C mirror 08F6 is 39945 while native is 39944. The recorded inner BBE9 entry shows current 0928 changed to 39944 across an NMI, while snapshot 092A remains 39945. No after-state seeding, inserted delay, or fixture change was used. Outside those two 16-bit observations, 2,747,926 compared bytes match. The wording excludes both complete fields (four bytes); it is not a claim that all four individual bytes differ. The unqualified original FAIL is retained.
- **Native pause timer/callback identity.** C game clock, actors and visible pause output hold. Native ED0D/EDAC timer decrement precedes the live-state clock gate, and no paused callback-disable was established here. Do not call C's held 08DE=300 a native state match. Return resets the timer; interrupt/fade/menu scheduling is separate.
- **Native VRAM upload/scanout, CPU/register/DP, and NMI timing.** The report records visible-map scanout differences and twelve crossing calls. A visible C image and matching working data do not establish those timelines.
- **Statistics, advertisements and complex foul clear.** Actual unsupported children/pending PCs and diagnostics remain; gameplay continues without a fabricated ordinary-panel substitute. Their complete rendering or continuation is not approved.
- **General strict-verifier mutation certification, complete cold-boot/audio journey, and normal human gameplay enabling.** None was completed or authorized by this review.

No author source, frozen candidate, original ROM or capture was edited by this audit. The useful bounded repair is accepted with these limits; root integration and its fresh build remain separate work.
