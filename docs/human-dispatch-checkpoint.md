# Bounded human caller stages, 2026-08-31

This follow-on adds **new files only** after the frozen controller checkpoint.
It leaves those19 reviewed source/doc files, the production source manifest,
the old probe binaries and the human enable policy unchanged. It is not yet
production wired and does not establish playable human basketball. Parent
integration and independent review remain required.

Worktree:
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-controllers`.
All evidence here is in its private `build/human-dispatch/` directory. The
earlier controller patch is described by `controller-implementation-checkpoint.md`
and frozen in `build/controller-contract/final-v1/manifest.json`, SHA256
`63c1f337303e23b08a6644794a23b5fee1e4c762ef0997c7f22b443c033e7184`.
Source hashes were rechecked after the follow-on work; none changed.

## What the new module does

`nba_human_dispatch.c/.h` separates three native caller stages without
interpreting opcodes, executing an emulator, or changing actor ownership:

| Stage | Implemented contract | Excluded continuation |
| --- | --- | --- |
| `87:9138-915D` input gate | Signed actor controller, selected record's processed latch; publish or skip route | Actual pad polling, input publication and pause/Select requester handling |
| `84:E2AC-E2F1` B prefix | Newly pressed B selects `84:DF7A` pass, `84:E141` switch, common return, or the other-button continuation; preserves owner/receiver/free-throw/inbound movement gates | Pass and switch children, and every `E2F2` non-B branch |
| `87:91C3-922D` post-action movement | Free throw, recovery, flag, receiver, inbound group/layout/actor/mode gates, followed when allowed by the existing production `85:A82C` velocity helper | Human actions before the stage, behavior dispatch after it, latch marking and physical actor commit |

Movement input is explicitly the state **after** `84:E2AC` returns. A native
action can change those inputs. The probe does not use a post-action native
snapshot as a substitute for implementing a whole C action: each stage is a
separate bounded native-prestate replay with its own stated boundary.

The native actor loop still dispatches normal behavior after human action and
movement. Removing human actors wholesale from the CPU/behavior scheduler would
not reproduce `87:922E-925C`. Likewise, marking a pad processed before the
action's possible control transfer is not the native `87:9276` ordering.
Those continuations are not implemented in this follow-on.

Native quirks preserved in source comments include the wrapped16-bit sign test
after CMP `$80` in the movement gate, sign rather than range testing of the
receiver in the B branch, and the existing accelerator's `85:AA1B-AA1D` cap
comparison: it loads the expired loop counter from DP C2 and compares that
with owner093E. Do not replace it with a comparison to the current actor index.
Corrupt controller indices >=5 or direction >8 return an explicit invalid
result, outside the supported native domain; arbitrary-memory parity is not
claimed for those inputs.

## Original sources and fresh native captures

ROM: `F:\Games\SNES\NBA Live 95 (USA).sfc`, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen source:
`C:\Users\joshs\AppData\Local\Microsoft\WinGet\Packages\SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe\Mesen.exe`,
SHA256 `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.

Each run uses a new portable process/home, empty private saves, explicit
controller1/zero RAM, fixed settings and its own subprocess environment.
Only normal pad input is written. No ROM, RAM, CPU or savestate is patched.
The manifest retains executed Lua/runner/helper copies, source/artifact hashes,
initial/persisted settings, observed Lua home, final save hashes, exit0 and a
completed boundary count. The verifier rechecks these, including exact final
settings/save hashes, before running C.

Raw snapshot format is explicitly sparse, **not full WRAM**: concatenate ranges
`0000..00FF`, `0500..09FF`, `1600..18FF`, `3400..49FF`, totaling7,936 bytes.
These include the stage inputs, controller/actor/context records and descriptor
pointer. Movement profile `+42` is read independently from the attested original
ROM using the native published DP E0/E2 pointer. Missing sparse input bytes
cause a probe failure rather than implicit zeros.

The capture records actual execution PCs for publish/skip, B pass/switch/return/
other, accelerator entry and movement exit. Gate and B expected results are
those observed PCs, not a second C-like decision function. Movement compares
the observed accelerator call and all128 current-actor words,160 controller
words and128 context words against raw native exit. C changes only velocities
and boost through the existing helper. No native expected poststate is passed
to the C probe. Numeric types, field sets, vector sizes, result counts,
duplicate JSON keys, complete paired stages and process status are checked.

Final captures both run1,800 court frames through normal menus:

| Capture | Gate / B-prefix / motion calls | Exact compared values | Manifest SHA256 |
| --- | --- | ---: | --- |
| `selection0-v2` | 8,570 / 786 / 786 | 337,118 | `cb1e4f62cdb2366b6daf5e141d5a150d0c71c34dfd7492e1647e0ec214cb238e` |
| `selection2-v2` | 8,570 / 857 / 857 | 366,796 | `ec0a83297c6b782ecb61d0e8ccdd8b495d8f847b500d8f5e1bf08a852c4ba516` |

Both final reports pass with zero mismatches/tolerances/skipped stages. They
retain25 left and4 right frame crossings rather than filtering them. These are
repeated field comparisons, not percentages or a complete branch-coverage claim.
The right and left counts differ because these are separate real native
journeys, not artificially synchronized fixtures.

Both routes exercise defense inputs and actual offense inputs. Left reaches
native controller-owned inbound offense at court844 and the B prefix calls
`84:DF7A`; right starts its offense input sequence at859 and calls the same
child during live offense. L, A, X and Y offense inputs are captured, but their
action branches explicitly return the unimplemented `E2F2` continuation in
this module. Native execution continues those children; the bounded C stage
does not pretend to implement their effects. Defensive B during the tip
selects `84:E141`. Detailed phase/button coverage is retained in each report.

The first left capture (`selection0-v1`) waited for live controlled possession
before issuing offense inputs. Native controller ownership arrived during an
inbound, which required a B press to become live; its offense sequence never
started. That evidence is retained. Version2 accepts native controlled inbound
possession after the tip and supplies only normal buttons. It does not inject
an owner or reuse a native state as a starting savestate. The first verifier
attempt also rejected one extra output line from the existing ROM loader;
the revised verifier accepts exactly that fixed diagnostic and still requires
one typed C result per captured stage. No expected native value changed.

Fresh `reference-v1/` contains bounded original-ROM recompiler emission and
Ghidra65816 listings for84/85/87. Manifest SHA256
`f561028c1144b794bdcfd8d51a7bf2f0f86c7944b67024a5dd04b7c15ada000d`
also hashes recompiler Python sources and the actual Ghidra65816 processor
files, launcher, version properties, scripts, original banks, commands and logs.
Tool locations are the same explicitly named installations in the controller
checkpoint. Original byte-range hashes:

| Bounded range | SHA256 |
| --- | --- |
| `84:E2AC-E2F1` | `dd22353ddb7bf83945d6f4b7f9155ab327ed206d54ea814b98394cab4b6e9460` |
| `85:A82C-AB16` | `49db1e33163bc56f8dabd6912dca0184898cd7f3952fafcdfe61d1ea36202c60` |
| `87:9138-9164` reference including publication calls | `ee4ce8dd82c6a1076952af6673de5635648c7b3533fe584f2377a61657181648` |
| `87:91C3-922D` | `3077898a97fab94a1c49ad972768681adae9277942fbab4b2a5a9ae07fc08c2e` |

## Build, acceptance limit and next caller

`tools/build_human_dispatch_probe.ps1` compiles only the new module/probe into
`build/human-dispatch/`, linking the previous frozen objects read-only. It does
not add the module to `nba95_sources.txt` or overwrite frozen binaries. The
final warning-free probe SHA256 is
`1f89b0241214852bbc470af0622c4a48cacc8c8fb4fbe85df86614447da79517`.
Run `tools/verify_human_dispatch.py --capture <capture> --probe
build/human-dispatch/human_dispatch_probe.exe --rom <original ROM> --output
<NEW report>`; reports and probe stdout/stderr are preserved without overwrite.
Capture reproduction uses `tools/capture_human_dispatch.py --output <NEW dir>
--selection 0|2 --frames 1800 --rom <original ROM> --mesen <Mesen source>`.

The earliest intentionally missing normal selected-human contract remains the
production initializer's effective-neutral policy. After that gate is lifted
only with reviewed complete routing, the next missing caller contract is the
requester stage `87:9165-91BF` (Start/Select, FT brightness/prepare child), then
the actual `84:DF7A`/`84:E141` children and other-button `84:E2F2-E3E9` branches.
Movement is implemented at its bounded post-action stage, but there is no
whole C actor-loop/action trajectory here. Processed marking, behavior return
register effects, natural FT/lifecycle/pause reallocation and multipad support
also remain. Do not enable human gameplay from these direct native-prestate
results alone.
