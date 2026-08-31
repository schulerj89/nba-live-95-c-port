# Human pass launch: independent source review and verifier rejection

The frozen C component passes this bounded source review. **The freeze is not accepted as a verification checkpoint:** both the natural verifier and its controlled-native companion accept a multiply return branch that the entry operands cannot take. No production enabling, source repair, native fixture change, commit, or whole-human-play acceptance was performed.

The reviewed controller freeze is `build/human-pass-launch/freeze-v1.json`, SHA-256 `87326e441bd90f6e8b921075724683e40e7b1cf94ad69b6949c471a5cbfe45a8`. Independent preparation checked 291 direct identities and 78 entries in the reference/tool closure. These counts are separate inventories, not a claim that all 369 paths are unique. Private audit outputs are under the auditor worktree's `build/human-pass-launch-audit-v1`. The original freeze and all rejected evidence remain intact.

## Findings requiring a verifier revision

The source starts at `85:F78B`. Its `CPX #$8000` and `CMP #$8000` distinguish the original signs of X and A. Opposite signs return through `85:F7AE`; equal signs return through `85:F820`. Zero operands do not remove this route requirement. A validator must derive the required exit from the entry operands before trusting the exit tag or using that tag to compute expected status flags.

The frozen `tools/verify_human_pass_launch.py` accepts either multiply exit tag in the supported sequence at line 225. Lines 254–257 then derive return NZ using that unbound tag. The following parsed-metadata substitutions retain original raw bytes and every C comparison but incorrectly pass:

| Natural call | Actual entry and return | Forged accepted return |
|---|---|---|
| First positive multiply, rows 12–13 | A=`0018`, X=`001F`; unsigned `F820`, result A=`02E8`, X=`0000`, PS=`00` | Signed `F7AE`; tag and PC/bank fields changed consistently |
| First negative multiply, rows 14–15 | A=`0018`, X=`FF9F`; signed `F7AE`, result A=`F6E8`, X=`FFFF`, PS=`80` | Unsigned `F820`; consistent PC/bank fields and PS=`00` |

The controlled companion `build/human-pass-launch/verify_controlled_math_v7.py` line 58 likewise accepts either return tag. Its case-specific PS assertions do not bind that tag to the actual route. Both the positive-low-FFFF and negative-low-FFFF controlled captures accept the opposite tag and matching PC/bank fields while all 5,670 C values still compare successfully.

These are verifier defects, not C mismatches or original-game bugs. The mutation tools alter parsed observations after original artifact hashing; they do not rewrite capture files, hash manifests, raw memory, or C output. The required rejection therefore tests whether the verifier semantically checks the route it claims to verify, separately from external freeze integrity.

Retained independent reproductions:

- `tools/test_human_launch_return_route_audit.py`, SHA-256 `b8765c160488b8a2c82931a626d2ab15d156c904a6fe686962d5139f8847360b`; `independent-route-v1/report.json`: valid baseline passes, both invalid cases incorrectly pass.
- `tools/test_human_launch_controlled_route_audit.py`, SHA-256 `b51e733f9bf7650f8798fd16a1f237655aee06adef69bc61b58ebd0396674a96`; `independent-controlled-route-v1/report.json`: valid baseline passes, both invalid cases incorrectly pass.

A separate verifier revision should reject all four substitutions, retain original valid outputs, and retain both original rejection reports. No change to the frozen C arithmetic or native evidence is indicated.

## Independent build and evidence

Only the frozen module and probe were freshly compiled in the auditor worktree using MSVC `/W4 /WX`; no old linked objects or asset loader were borrowed. Original disassembly/reference identities were checked, and the independent diagnostic reads the original ROM directly.

| Object | SHA-256 |
|---|---|
| Original ROM, 1,572,864 bytes | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| `src/nba_human_pass_launch.c` | `2d9c7109e4fe1a79fd50011d865b14710921e3040463b6a051ead60b2ac42829` |
| `include/nba_human_pass_launch.h` | `6ccb7abe0b2fcc8bddefcaa3f424b732baf571d3c412e3cee76d28da77fc420d` |
| `tools/human_pass_launch_probe.c` | `beb6e69ca5c7f1259b3fd3b67f1b7ffadc9b63048aaa8612e3688ac459fb061c` |
| Frozen natural verifier | `2981b9cdd9c4c5d021cdc290cb6cfb38da3afa9a01e1ffc9539349ba5cf6fc37` |
| Frozen controlled verifier v7 | `0a68d386857ab8566e75598e985c5ae82bdf13e2f54f569bfe522c747afd88ae` |
| Fresh private executable | `3e6214b3a29685dbfc5e44f368bf04d3a5e34568adca2efa539c7da8c11b9bad` |

| Independent run | Result and limit |
|---|---|
| Original left natural capture | 13 launches, 26 multiplies, 26 divides; 122,811 compared values |
| Original right natural capture | 17 launches, 34 multiplies, 34 divides; 160,599 compared values |
| Original final controlled-native v2 captures | Three multiply calls; 5,670 compared values |
| Original negative suite | 177 cases pass; does not detect the route defects above |
| Original source edge suite | 14 guards pass with the fresh executable |
| Independent arithmetic ROM diagnostic | 754 cases, 1,425,060 values, 155 executed source PCs; no differences |
| Independent whole-launch ROM diagnostic | 108 cases, 203,796 values, 462 executed source PCs; no differences |

The two natural stdout/stderr pairs are byte-identical to the frozen original reports, recorded in `preservation.json`. The fresh controlled run used the original verifier with only its probe and output destinations redirected into the auditor's private directory; the transformed runner is retained. No frozen output was overwritten.

`tools/test_human_launch_math_rom_audit.py`, SHA-256 `fde503eb88c7c19b8164704524e478de264a6a541d3e8f7d7d42bf0b1dbb0674`, independently implements the bounded original opcodes and width transitions, rather than calling the component or the implementer's arithmetic reference. Its edge Cartesian cases and deterministic varied operands cover multiply/divide signs, `0000`, `FFFF`, `8000`, and quotient/remainder extremes. Final evidence is `independent-math-v2/report.json`; earlier diagnostic evidence is retained.

`tools/test_human_launch_full_rom_audit.py`, SHA-256 `9358aaaddbd2a34196601382897cf3820ffdc6ed4b73343935a5ac8e89c0ace4`, uses that independent opcode diagnostic to execute the complete bounded launch and cleanup. The 108 cases cover three families × six table bands × source/receiver alias or distinct × three upper descriptors. They vary full-word positions, velocities, height, live state, modes, actor selections, and carried fields. They compare all probe vectors, including unchanged fields, not only final ball velocity.

This diagnostic supplies the source-defined 8×8 multiplier result when the original instructions read its result registers. It does not model hardware latency, master cycles, physical CPU stack bytes, interrupts, or general SNES execution. Its controlled cases establish the bounded source memory contract, not natural reachability of these branches.

## Source contract and original behavior preserved

The API is binary 16-bit arithmetic with valid typed actor indices and the original three six-row ROM tables. Valid bands are `0,6,12,18,24,30`; invalid indices or bands refuse before changing state. The probe binds original `96`/`8E` pointers to the selected actors. Source/receiver aliasing remains possible and is exercised by the independent source cases. The native verifier checks decimal clear, 16-bit M/X, DP zero, and the captured DBR where applicable; the C API does not claim complete CPU state.

At `85:F7C9–F820`, the overlapping byte stores leave the cross-product sum's high byte at `0822`, not the returned product's high word. The implementation preserves this arithmetic scratch effect and increments `085A`. `F7D4 SEP #$30` clears both index high bytes; the later REP does not restore Y. All 30 natural first launch multiplies witness actor-pointer Y becoming `00EB`.

At `F7A5/F7A8`, EOR/BEQ skips the normal low-word increment when magnitude low is `FFFF`. The resulting signed product is one less than ordinary two's-complement negation: controlled A=`00FF`, X=`FEFF` returns `FFFF0000`, not `FFFF0001`. The original instruction behavior is retained and explicitly commented in C lines 25–28. This is a declared controlled-native witness, not evidence that normal gameplay reaches those operands. The separate low-zero case follows INX and must not be merged with the low-FFFF path.

`F867–F8D8` performs unsigned division after the `F8D9` wrapper's magnitude/sign preparation. Its zero-denominator branch returns zero quotient and the original magnitude as remainder. The component preserves that result rather than invoking C division by zero. The wrapper returns signed quotient low in A, remainder low in X, and magnitude quotient high in Y; X is not a signed quotient high word. Its DP and WRAM arithmetic scratch outputs remain exposed and compared.

At `86:99C4`, the ten PEI scratch words are saved and restored. The component copies profile E0/E2 to `0914/0916`, publishes release `094A=1`, clears owner to `FFFF`, sets source delay 20, selects ball pointer `3EEB`, and takes duration and height from the actual selected ROM row. The third word in each table row is not read by this routine.

Predictions multiply signed duration/velocity, arithmetic-shift by eight, and wrap coordinates to 16 bits. `9A8D/9A95/9A9F/9AA7` branch on the wrapped CMP sign, not host signed ordering. Clamp children also change the receiver's actual velocity. The independent cases exercise this distinction, aliasing, and all four clamp directions. Ball velocity derives from a sign-extended wrapped delta shifted by eight; it does not subtract unrestricted host coordinates.

`9C45` advances integer ball positions with six arithmetic shifts for source upper `2B/2C`, seven otherwise, preserving fractions. Source Z is compared unsigned with 16 before reducing initial vertical velocity by `0080`. Receiver mode 14 retains its timer; other modes receive duration plus 15. Source mode 15 skips `9846`; otherwise that child selects mode 1/2 from group/offense and clears the original flags/timer while setting behavior 47. The final live-state test also uses wrapped CMP sign. These rules and their memory effects match the independent original-instruction execution.

## Provenance, interrupt observations, and exclusions

The normal captures use isolated Mesen settings/saves and controller inputs. Their capture Lua has no WRAM, ROM, PC, or register mutation. The exact natural Lua SHA is `b71ecca6cf1a8952c8b2387ee17e5c2136b017759d29e8ea19068eadeb375d0c`; runner SHA is `0a281da9bab1f994c5962546d037a35e4e9c232f97f3cacd4519681c97218020`. The observed human initializer/return and mode-15 dispatch boundaries are verified components of these recorded routes, not an implemented continuous human journey.

The final controlled v2 captures preserve the natural first eleven rows and their raw files, then write only declared B2/B6 operands at `869A36` before the original LDA/LDX. They do not patch ROM or force PC/register values. They stop after the first multiply. The old controlled v1 captures with orphan raw files and `capture_error` remain failed history; superseded early PASS reports are not used for acceptance.

Two natural left-call interruptions have direct `80815A` entry and `80859B` normal RTI observations, at court frames 935 and 1002. Captured hardware frames identify interrupted PCs `869A76` and `85F888`; SP and saved registers restore. `independent-nmi-observations.json` records their changed DP bytes. The verifier's exclusions are restricted to observed, unowned DP differences that the NMI before/after chain explains; owned launch scratch may not be exempted. It still proves the C result retains each such original entry byte. No child or interrupt poststate is provided to C. This does not prove instruction-by-instruction interrupt writes, reentrant NMI behavior, or timing.

No production manifest or human enable changed. Natural evidence does not cover the controlled clamp, divide-extreme, alias, alternate-upper, height, or cleanup branches merely because source diagnostics cover them. This review does not accept a complete initializer-to-release journey, hardware multiplication timing, CPU stack/register parity beyond the declared observed fields, or whole-game parity. The next acceptance step is limited to reviewing corrected verifier-only files against the same frozen C and native captures.
