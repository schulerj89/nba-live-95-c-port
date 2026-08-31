# First-NMI OAM-address prefix checkpoint

This child continues the frozen normal power-on/NMI packet from `80:8184`
through the first completed OAM-address write and the source-produced branch.
It stops at `80:818E`, **before** reading PPU status register `213E`. It does not
provide a PPU status value, controller sample, DSP value, OAM data upload, RTI,
host scene transition, or full S1/03DB completion.

The prerequisite is `build/bootstrap-first-nmi-freeze-v1.json`, SHA256
`a9ed245ee2cf1f2c64b8fd40e4be2276e628a9d6efae47470dd444e773c53b59`.
All 4,567 prerequisite identities remain exact. The child adds a continuation
scheduler over the carried CPU and console owner; it neither edits nor copies
the frozen owner at the boundary.

## Owned source work

| Source | Effect |
|---|---|
| `80:8184 8F 03 21 00` | Store the carried low accumulator byte `80` to `2103`. |
| Pinned PPU `2103` semantics | Publish priority rotation from bit 7 and reload the internal OAM byte address from the source-written `2102/2103` address. |
| `80:8188 AF FE 08 00` | Read actual mirrored WRAM `0008FE`; reset source left it zero. |
| `80:818C D0 1A` | Use the real zero flag. The normal branch is not taken. |
| `80:818E` | Explicit source stop before `AF 3E 21 00`, the unowned PPU-status read. |

The new `2103` write supersedes the earlier internal OAM increment history.
The component therefore derives the post-write address from the existing
source-written `2102` byte and the new value; it does not initialize the prior
internal address from the capture. CPU writes do not alter the memory-manager
open bus. Automatic-joypad state remains carried and lazy; no controller action
is reached in this prefix.

The continuation validates the exact canonical source bytes while executing
real bus fetch/read/write cycles. The verifier independently decodes every CPU
instruction from the canonical ROM and compares the ordered native bus route,
so the small source-specific runner is not a captured instruction transcript.

## Fresh normal evidence

`build/native-bootstrap-oam-prefix-v1` is a fresh isolated normal cold boot
using a private Mesen binary, settings directory and saves directory. There are
no inputs, state injection, ROM patches, or inherited saves. Manifest SHA256:
`dd83f22640c60e41930d58eb553cd3c0971cff9d47943768e062b563304722d1`.

The actual normal boundaries are:

| Boundary | Master | CPU cycles | A | PS | Priority | Internal OAM |
|---|---:|---:|---:|---:|---|---:|
| Before `8184` | 2,094,260 | 226,159 | `FF80` | `A1` | false | 2 |
| After `2103`, at `8188` | 2,094,330 | 226,164 | `FF80` | `A1` | true | 0 |
| Before `213E`, at `818E` | 2,094,374 | 226,171 | `FF00` | `23` | true | 0 |

Fresh `/W4 /WX` executable SHA256:
`b6d2b7c34f963c5a9bbd7d0635a4a19395da7a0accd983ea198e4e60b505849e`.
The strict report passes 69,901 CPU instruction entries, 32,719 CPU data
accesses, 131,072 DMA operations, 24,592 native-observed SPC instruction
entries, 11,708 SPC writes/I/O, 1,966,080 boundary WRAM bytes, full entry/F1
ARAM, full first-fill VRAM, and 25 terminal typed fields. Source endpoint:
master 2,094,374; CPU 226,171; source SPC ticks 199,962 at `03CD` phase 0;
1,536 refresh stalls. Native SPC observation remains lazy, so this is not joint
callback parity or full DSP state.

The isolated contracts pass 21,535 assertions over boundary refusal, all OAM
address bytes, all `2103` values, all loaded `08FE` byte values and the explicit
absence of `213E`. Ten new reached corruptions, the inherited nineteen NMI
corruptions, the twelve terminal/output guards adapted to `818E`, and the prior
independent ten plus four parsed-view cases all pass. The unadapted old terminal
test intentionally expects `8184`; its failed draft run is retained outside the
freeze and is not acceptance evidence.

## Integration boundary

Copy the new delivery files exactly, compile `src/nba_bootstrap_oam_prefix.c`
**instead of** separately compiling `src/nba_bootstrap_nmi.c` because the child
privately includes the immutable prefix implementation, and do not add it to
the production manifest yet. The next owner must implement the actual `213E`
status/open-bus semantics before choosing the branch, then continue the OAM
publication/DMA handler with carried NMI budget and controller timing. It must
not seed the observed status, OAM address, controller data, or SPC continuation.
