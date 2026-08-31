# Period entry prefix independent acceptance

**PASS for the bounded DCA6-to-DD97 data prefix and its verifier.** The original continuing-period carry is preserved, the typed input comes solely from the DCA6 prestate, and no native after-state, phase constant or child result is supplied to C. No production enabling is accepted by this report.

The controller worktree freeze `build/period-entry-prefix/freeze-v1.json`, SHA-256 `a96b329445557aaa0e4d6976e97a14b9f518d0cbd55ccaaed4ff36893b3e3ea8`, was independently checked: all 254 direct freeze entries match. Another 74 disassembly/reference artifacts and tool-source dependencies named by its reference manifest match. Its exact accepted native reader and native freeze remain pinned. Exact private snapshots and reports are under auditor `build/period-entry-prefix-audit-v1`. No original source, capture, previous freeze or rejected evidence was changed.

## Actual caller and source

Raw ROM inspection confirms `$87:976E` increments `$0926` before the continuing-period path. `$87:9797` jumps to `$8C86`, and `$87:8CA6` calls `$86:DCA6`. This path does not invoke the new-game DA18/DA3F clear. Earlier `$8C86` work and optional sound/wait calls remain outside the prefix. The raw caller and all 241 prefix bytes are retained in `raw-rom-caller-prefix.txt`.

The prefix has no JSR/JSL within `$DCA6–DD96`. Its C reset clears only the specified global words and twelve words per actor, sets the two queue cursors to FFFF, and leaves queue contents, positions, fractions and other owned-state fields intact. The fixed actor loop is ten records at `$34EB + i*$100`. The explicit `$4015/$401B/$1864` writes and context transient clears match their original instruction operands.

At `$DD2D–DD46`, original CMP #4/BCC is unsigned. Period words below four preserve selected clock `$0A0C` and copy it into `$0928`; quarter option `$17B1` is not read on that route. Periods at least four select the original `$86:E392` table: 7200, 10800, 14400, 18000. The typed API explicitly refuses out-of-table overtime options before the combined reset mutates state. That refusal is a host domain boundary, not an invented native branch; the original could otherwise index beyond the represented table. The downstream parent separately bounds its period domain.

At `$DD47–DD96`, shot clock `$092C` and `$0994` become `$05A0`, `$0996` becomes one, and only period two negates both context anchors. The EOR FFFF/INC behavior wraps at 16 bits, preserving zero and $8000 results. The current first anchor is copied into DP `$B6`, owner `$093E` and assistance `$09C0` become FFFF, and the list cursor/sentinel become `$34D3`/zero. The source deliberately carries `$09BA` and `$09B0/$09B2`; the C/header comments preserve this continuing-period behavior instead of applying a generic restart finalizer.

The implementation owns data, not emulated CPU state. Its binary16/DP0 and effective WRAM absolute-bank domain is explicit. Actual boundary A/X/Y/P values are checked separately. In particular, DD97 has A=$34D3, X=0, Y=$34EB; Z follows LDX #0 and carry follows the original CMP(period,2). The source diagnostic below also validates the reset and clock boundary flags directly.

## Fresh independent verification

The auditor freshly built only the exact new C module/probe with `/W4 /WX`; there were no compiler warnings or borrowed objects. Four unchanged controlled-expiry native captures match all 65,536 WRAM words at each of DD2D, DD47 and DD97: 786,432 word comparisons. This includes memory outside the typed projection. All eight stdout/stderr artifacts are byte-for-byte identical to the original frozen replay; `preservation.json` records their hashes.

The original 90 controlled source cases (14,310 typed words) and 56 integrity mutations pass against the private build. The source cases include invalid overtime indices, high period words, regulation quarter values that must remain unread, varied carry values and wrapped anchors.

The independent `tools/test_period_entry_prefix_rom_audit.py` does not import the implementer's fixed-block reference. It executes actual original opcodes in a bounded binary16/DP0 memory diagnostic, including CMP, ADC overflow/carry, EOR/INC and actor-loop register updates. It visits 87 source PCs. It reproduces all 786,432 native words and 48 A/X/Y/P boundary fields. Another 72 independently generated poisoned-WRAM cases match every word at all three C boundaries, or 14,155,776 controlled word comparisons. These include arbitrary unowned bytes, ready/dead words, all valid overtime choices, high periods and signed anchor extremes. This is an isolated source proof, not a normal-play or CPU timing claim.

The independent `tools/test_period_entry_prefix_protocol_audit.py` exercises the actual native verifier and independently asserts the exact executable/ROM/DCA6-prestate command. Its baseline passes; all 17 malformed inputs reject before C: duplicated/reordered prefix rows, wrong boundary registers/carry, decimal entry, extra/native-type metadata, and missing/extra/malformed build identities. The already accepted native reader supplies exact complete hook topology, numeric domains, raw-field binding and capture isolation. The prefix verifier additionally checks frozen raw identities, period/quarter provenance, clock/stack continuity, original return-frame bytes and derived CPU boundary values. It does not rewrite attestation.

## Identities and limits

| Object | SHA-256 |
| --- | --- |
| Original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| C module | `5b91d9c6a865ab9c7cf69df31ac74f5a7dd6b396964b541cff4c0be3030967ea` |
| Header | `db93c79b82b7c1a5696b1937f57a60ca31f78ab4d204d39b6ab44086cd954a26` |
| Probe | `2d86bfa683506db7ab5e4b535080043093f6ea40d6aa5cf1e342610d1366d1af` |
| Verifier | `eb106d5bb668f554925774f71639efc382a86dcaa56e759ce173138f5f3961f0` |
| Fresh private executable | `788b12cbe10c55ab25ff3ea757fb1dc804694f2fd0f98c64eacc43b7c327932c` |
| Independent ROM diagnostic | `5362cb316d5f802cf8e7d7ed46660aa6216cefce75b3c4939d49655562da0063` |
| Independent protocol tool | `9f59af06e33bbc0950c0428cac740dfb017d7c44a47d379a5c9daa977a07ce26` |

The four native witnesses are controlled expiry cases after ordinary boot/play, not naturally elapsed full periods. They witness carried ready=1 and dead coordinates zero; nonzero dead-coordinate preservation is source-tested only. No DBR, emulation flag or cycle counter is captured. Full data endpoints and observed boundary flags do not prove CPU/register continuation, real-time interrupt scheduling, earlier period handling, UI/drawing, a complete restart, normal scheduler phase or human play. Integration must provide the already incremented period and original clock table from owned state and pass the resulting DD97 state into the separately accepted parent; it must not seed from native DD97.
