# Paired period role-prefix ownership

`nba_period_roles.c` implements the two calls made by `$86:E1E5–E1F6`, each entering `$85:BC07`. It owns the initial five-actor pair/focal geometry scan, `$09DA` nearest-pointer publication, and the actual carried cadence gate. It includes the original `$85:F34F` geometry child. It stops explicitly when the remaining rebuild or planner is required; it does not call the existing single defense end-frame wrapper.

The new files are private and pending independent acceptance. There are no production manifest/audio/main changes, captures, commits, or root-worktree edits. Earlier freezes remain unchanged. This is a source-state helper, with no opcode interpreter, delay, cycle model, response replay, or normal-state snapshot initialization.

## Source order and focal point

The paired caller is:

1. `$E1E5 LDA #$46EB`, `$E1E8 STA $9E`, `$E1EA JSL $85:BC07`.
2. `$E1EE LDA #$476B`, `$E1F1 STA $9E`, `$E1F3 JSL $85:BC07`.
3. `$E1F7` begins the separately owned `$86:D5DB` sort and is this helper's completed-pair boundary.

At `$BC07–BC0E`, the entry context is preserved in scratch `$A6`, and its `+02` opponent context replaces `$9E`. Therefore the first call scans actors 5–9, then the second scans actors 0–4. This ordering does not depend on camera side. The represented initialized context records are `$46EB: opponent=$476B, firstActor=$34EB` and `$476B: opponent=$46EB, firstActor=$39EB`.

`$BC10` tests the sign bit of the full owner word `$093E`. If nonnegative, `$BC15` dereferences **the object pointer `$0910`** and uses that object's integer `+04/+08` words. In this bounded period domain that pointer is `$3EEB`, the ball record. It does not look up the owner's actor XY. With negative owner, `$BC26–BC2E` uses the carried prediction `$0918/$091A` and leaves scratch `$8E` unchanged. Rounding actor or ball fractions would change these inputs.

## Pair geometry and original arithmetic

For each scanned actor, a negative `+74` assignment skips the pair child and preserves its pair fields. Otherwise `$BC49` looks up the even assignment index in the ROM table `$87:9C7B`. The bounded projection accepts even indexes `0..18`, mapping to the ten records `$34EB..3DEB`; other nonnegative indexes are rejected as outside the represented record domain, rather than rounded, masked, or silently ignored.

`$BC52–BC62` forms wrapped 16-bit `(pairedX-currentX, pairedY-currentY)`. `$BC64 → $85:F34F` returns scratch `$AA` distance and `$B2` direction. `$BC6E/BC71` publishes distance to both records. `$BC79` deliberately skips both direction stores when direction is 8; otherwise the current record receives the direction and its pair receives `direction XOR 4`.

The geometry child is transcribed from `$85:F347–F3BA`, including its actual `$F09A` 16-byte table. It preserves the original arithmetic:

- Negation is 16-bit, so negating `$8000` remains `$8000`.
- `$F37D` compares by the N/Z result of **wrapped subtraction**, not a host signed ordering of its operands. Equal or negative enters the swap path.
- `$F394 ASL` truncates before the later logical right shifts.
- `$F399 CMP` likewise uses the wrapped N bit for the direction-table key.
- Even the source result `(dx,dy)=(0,1) → direction1, distance0` is retained and commented. It is not smoothed into a conventional octant or distance.

The separate focal-distance calculation `$BC84–BCBD` also uses wrapped negation, wrapped `CMP/BPL`, and two logical shifts. It is not replaced by the pair-distance routine. `$BCC4/BCC6` selects a new nearest pointer only when the wrapped subtraction from current best has N set. Ties keep the earlier winner. If no value wins, scratch `$92` retains its incoming value; `$BCDF–BCE1` still publishes that carried value to `$09DA`. `$09DA` is written **before** the cadence check on every call.

## Cadence and explicit unresolved paths

The caller supplies the current `$C6` delta; the helper never assumes it is 2, or that `$09D2` starts at 12. The implemented source transitions are:

| Source condition | Included writes | Boundary |
|---|---|---|
| `$09D6 != 0` at `$BCE4` | `$BD07–BD0A` sets `$09D2=30`; preserves `$09D6` | `REBUILD` at `$85:BD0D`, completed call count unchanged |
| `$09D6 == 0`, wrapped `$09D2-$C6` nonnegative | Store that subtraction | Original return `$BD06` |
| Subtraction negative, camera `$093A` negative | Add 30 once with 16-bit wrap; no re-test or normalization | Original return `$BD06` |
| Subtraction negative, camera nonnegative | Add 30 once | `PLANNER` at `$85:BE06`, completed call count unchanged |

After the first real return, `advance` yields `FIRST_RETURN` at `$86:E1EE`; `resume` permits the second call. After the second return, it yields `COMPLETE` at `$86:E1F7`, before sort. Repeating `advance` while waiting is immutable. Only `FIRST_RETURN` can resume. No caller state changes are allowed between the two calls because the original intervening code only installs the second context.

`REBUILD` excludes `$BD0D–BE03`, including its assignment loops and `$85:B95C` children. That remainder eventually clears `$09D6` at `$BE03`; this helper must not clear it early. `PLANNER` excludes `$BE06–C0F5`, including protected-basket geometry, mode/assignment decisions, and `$B9D2/$BA1D/$BAE4/$BB6C/$BBBF` helper calls. It must be implemented or explicitly dispatched before treating that call as complete. The helper does not claim that the observed early-return path covers those domains.

## Typed state and native proof

`include/nba_period_roles.h` exposes ten six-word actor records, two context records, ball/prediction/owner/camera inputs, the carried cadence/rebuild/nearest words, delta, thirteen named scratch words, and explicit ready/dead-coordinate preservation. `tools/period_roles_probe_fields.inc` gives all original word addresses. There are **91 typed words**. The API requires the source M/X flags clear, decimal D clear, and direct-page base zero; it is not a full 65816 register/stack model.

The probe takes exactly one 186-byte input: magic/version and those 91 current words. There is no expected-after argument, raw-WRAM loader, optional child return, or after-state adapter. The verifier extracts only `roles.before` into that input and uses `roles.after` only for comparison.

The four frozen parent captures are the enhanced `period-{0,1,2}-ready1-children-v2` and `period-3-ready1-children-v3` under `completion-owner/build/period-restart-attribution-v1`. They are the previously documented normal cold-boot/menu journeys with controlled expiry, not naturally elapsed full periods. All have `$09D2=12`, `$09D6=0`, `$C6=2`; cameras are `0,0,5,FFFF`. The implementation derives two ordinary returns and `$09D2=8` from those inputs. All **364 typed endpoint comparisons** pass, including pair geometry, focal distances, nearest pointer, all represented scratch, and preserved latches. No full-WRAM, CPU-register, timing, or whole-frame parity is claimed.

The verifier reuses and pins the independently accepted period-v2 native capture guard, SHA-256 `68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca`. That guard checks complete capture artifact/source/settings identities, exact route tags and CPU domains, declared seed changes, and immutable source files. The new gate additionally binds the adjacent `$E1E5/$E1F7` endpoints and verifies fresh build/source identities, strict typed output records, and exact parent-return order.

Sixty local checks pass: 18 cadence/rebuild/camera cases, 12 instruction-derived pair-geometry corners, four wrapped focal-distance/nearest cases, five owner-sign focal-source cases, eight malformed typed inputs, and 13 parsed subprocess/boundary/output corruptions. Cases include negative cadence, subtraction overflow, camera `$00FF` versus `$8000`, nonzero rebuild words, zero and `$FFFF` delta, direction-8 preservation, `$8000` arithmetic, and a scan with no winner preserving `$92`. These are source-only checks beyond the observed cadence; they do not establish native reachability of every synthetic state.

Fresh `/W4 /WX` build: `.analysis/period-roles-build-v3`. Native report: `.analysis/period-roles-native-v1/report.json`. Test report: `.analysis/period-roles-tests-v1/report.json`. Earlier private builds and development reports remain retained. Independent audit is required before production acceptance.

## Source identities and reproduction

The original unheadered ROM SHA-256 is `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

| Inclusive original interval | Bytes | SHA-256 |
|---|---:|---|
| `$85:BC07–BD0C` | 262 | `18d46ce022ac84463755e6492ac6bc6286e94afd1d306c38f8bebcae982da219` |
| `$85:F347–F3BA` | 116 | `55460ab6701df4ff6c8042be3171ded9502d0a06bf888f90923b28dd4a0b091c` |
| `$86:E1E5–E1F6` | 18 | `4e886e607715c0e34cb130c6a075dd05d8c07313febf393dc599862503e3b5ad` |
| `$85:F09A–F0A9` | 16 | `131b39628e8df2a8f80b417e318f6de3b53345ddab30127c89da018858d5bfcf` |
| `$87:9C7B–9C8E` | 20 | `b6bcb83495385ed171a3cad817d447694b540b8baf0dd17d203307b3630f3148` |

Source review used the primary repository's read-only bank85/bank86 listings and verified the actual ROM bytes at known 16-bit widths. The incomplete listing was supplemented with static decoding of `$F347–F3BA`; no emulator session was launched.

Run from the scheduler worktree with fresh output names:

```powershell
.\tools\build_period_roles_probe.ps1 -OutputDirectory .analysis/period-roles-build-recheck
python tools/verify_period_roles.py --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-1-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-2-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-3-ready1-children-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/period-roles-build-recheck/period_roles_probe.exe --output .analysis/period-roles-native-recheck
python tools/test_period_roles.py --verifier tools/verify_period_roles.py --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/period-roles-build-recheck/period_roles_probe.exe --output .analysis/period-roles-tests-recheck
```
