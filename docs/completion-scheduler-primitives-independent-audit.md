# Independent audit of bounded Setup scheduler primitives

**PASS for repaired freeze-v3's bounded source/primitive and verifier/probe
scope. Freeze-v2 was rejected for integrity defects; that evidence is retained.**
Production Rules reentry, producer elapsed time and partial-DMA scanout remain
unaccepted. This review did not integrate the module or edit the scheduler's
frozen files, original ROM or native captures.

The first candidate is
`.analysis/worktrees/completion-scheduler/.analysis/scheduler-freeze-v2.json`,
SHA256 `3d4f282797fad6672d3954b0c16e85300d1c716f8271c4cf3283c23448a14e0a`.
All14 declared files were hashed and size-checked before copying six exact
module/header/probe/build/verifier/test files into the auditor's
`build/scheduler-audit-v1/source`. The fresh standalone build uses `/W4 /WX`;
it reuses no game or implementer objects. Its executable SHA256 is
`48d3c7054f993d0db8071fab4dff599c24d34a0efcc1aaee23b79645b30632f8`.
The preserved copy remains the v2 review target after repairs began in the
scheduler worktree.

## Original source and caller review

I read the original ROM, Ghidra's bank80 captured listing and the generated
bank80 M1 wait routine. The original ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The reviewed byte spans and listing/recomp hashes are retained in
`build/scheduler-audit-v1/source-evidence.json`.

| Native owner | Verified behavior and scope |
| --- | --- |
| `$80:81F1`, `$821A-$8254` | Incoming palette test is M=1. It tests the low byte, then switches M=0 to transfer and clear the full size word. X remains8 for source bank and CGRAM address. The low-byte skip for size256 is preserved/commented. |
| `$8244-$824D` | `SEC; SBC size; SBC #$50` retains the first borrow. Palette publication precedes subtraction/clear. This edge is source/C evidence, not a naturally observed underflow. |
| `$825C-$82A2` | Mode1 records preserve source bytes1..3, full size/destination, two carry checks, inclusive exact budget, cursor advance8 with mask511, then DMA. |
| `$82E8-$832F` | `$FD` fixed low-port DMA, overhead120, VMAIN0 for DMA then restore128. |
| `$8332-$8375` | M=1 `CMP #$D6; BEQ $8378`; `$FC` high-port branch overhead119, no VMAIN write. Old mixed-width Ghidra `CMP #$F0D6` is incorrect. |
| `$8378-$83CB` | M=1 byte immediates set DMA1/port24, overhead138, VMAIN129 then restore128. This `$D6` branch has source/controlled C coverage only; none of the126 natural jobs use it. |
| `$82E3`, `$83CE` | Native memory head is published at budget/service exit, not between DMA jobs. Budget failure retains the stopped job's budget/cursor; previous successful jobs remain committed. |
| `$84A1-$84AB` | M=0 guard reads the complete `$059C`; zero permits the16-bit epoch increment, including wrap. |
| `$86B0-$86BE`, callers `$EF1A`/`$8959` | Wait preserves M. Header compares16 bits; `$8952` sets M=1 before `$8959`, so that caller compares the low byte only. The primitive masks the loaded value and blocks continuation during interrupt execution. Register saves, instruction timing and caller scheduling are not represented. |

Unsupported queue modes fail explicitly. This is a publication-intent API,
not an atomic whole-queue transaction: callers must treat an unsupported
result as a failure, since a preceding supported job may already have emitted
a publication. Palette/VRAM data bytes, pre-budget register side effects,
bus/refresh timing, OAM/HDMA, callbacks/audio and producer costs are excluded.

## Fresh comparisons and evidence

The freshly compiled auditor probe passes every current native observation:
599 queue services,126 DMA publications (102 copies,10 low fills,10 high fills,
four palette),12 budget stops, four M0 header waits and16 M1 waits. Final
head/budget/palette count, job order, DMA mode/port/source/size, actual native
PPU destination and increment/remap values are compared. Expected outputs
come from native observations; the probe receives queue prestates and wait
boundary epochs, not expected output arrays. Epoch inputs do not establish
source-derived elapsed time.

All7080 old V1 events and7102 V2 events retain their old fields/timestamps.
I independently compared all eight entry/after-wait WRAM snapshots across
V1/V2/V3 byte-for-byte. Native-v3 has trace SHA256
`1ce0158650dc48ff7a35a6238d6dce9af8b3d720cfbea56e3d8d596376ad0543` and
manifest SHA256
`e8bf2695885be8514b0de302e5c467966fe11ca6c6a985d0ada7d888339d6e43`.
The actual initial, persisted and manifest settings agree, including disabled
automatic patches, deterministic power-on and private saves. The capture
runner creates fresh folders and the Lua changes controller input only;
its normalization rebases labels without resetting the emulator. There were
no missing original artifacts in this audit.

All22 supplied C/integrity tests pass. Additional independent checks:

- `tools/setup_scheduler_audit_probe.c` observes queue head inside both
  callbacks during a wrapped two-job service, then checks complete and
  partial-budget exits. It also checks invalid-cursor rejection before a
  pending palette publication and M1 wrap/interrupt gating. PASS.
- `tools/test_setup_scheduler_audit.py` checks two wrapped multi-job cases
  and corrupts each of eight actual queue DMA output fields. All10 semantic
  checks behave correctly. These are engineering checks, not new native
  captures.
- The same independent tool exercises attestation omissions, incorrect
  scalar types/settings identities and malformed probe commands without
  writing any raw fixture. Freeze-v2 passes13/32 total cases and fails19.

## Freeze-v2 findings requiring repair

`verify_setup_scheduler.py:read_capture` only hashes present entries. Removing
the trace, observed-environment or sentinel artifact declaration still allows
the original file to be consumed without attestation. Omitting script, runner
or settings source declarations also passes. Boolean schema1 is accepted,
and changing the post-settings hash or declared automatic-patch setting is
ignored. These gaps do not invalidate the independently checked actual
corpus, but prevent accepting the advertised strict verifier.

`check_build` likewise accepts an empty source map, any missing expected
source, and Boolean schema/compiler-exit values. The fix must require the
expected source/artifact declarations and exact numeric domains, then compare
the actual initial/persisted settings and their identities.

`setup_scheduler_probe.c` uses permissive `scanf/sscanf` conversions. A queue
hex pair `1g` becomes mode1 and succeeds. `load 4294967296` wraps to zero and
succeeds; `load -0` also succeeds. Reject malformed/partial tokens and range
overflow before conversion. These are host protocol defects, not original
game quirks, and none entered the valid native replay's generated commands.

The findings were sent to the scheduler owner for a new freeze. The original
v2 sources and failed mutation report are retained. The accepted repair did
not change the module/header, native fixtures or semantic comparisons.

Auditor evidence: `build/scheduler-audit-v1/native-replay.json`,
`edge-integrity.log`, `independent-mutations.json`, `source-evidence.json`,
the source copy, fresh build manifest and independent API probe/compile command.
No completion percentage or Rules timing acceptance follows from these checks.

## Accepted freeze-v3 replay

Final frozen manifest SHA256:
`55a02890409ff992ad6fc167d33d2f9c11ecc56f2a989c86f63cb2bad28b5dec`.
I verified every declared file identity before copying the exact six source
files again into `build/scheduler-audit-v2/source`, then rebuilt the standalone
probe from those copies. No objects from the rejected build were reused.
The fresh auditor executable SHA256 is
`1059b122516f3e130d1279a5269dc586843e9e45b78539b24c0efa2582ca3e68`.

The native comparison again passes599 queue services,126 publications,
12 budget stops, four M0 waits and16 M1 waits, while preserving7080 V1 and7102
V2 events. All32 revised supplied tests and all32 independent mutation/semantic
cases pass. The original19 failing independent cases now reject the invalid
input or attestation. The independent test was not changed to obtain that pass.

Source inspection confirms required capture/build/source/artifact sets and
exact scalar domains are enforced. Declared paths are tied to their named
files, actual initial/persisted settings and hashes are checked, native row
fields and PCs are validated, and the new line-based probe parser rejects
partial hex, signs, numeric suffixes, overflow, embedded NUL and wrong arity.
The bounded native output comparisons remain intact. Additional verifier
support for separate interrupt-tail capture files does not imply this audit
approved that capture or a production interrupt-cost model.

Final source identities:

| File | SHA256 |
| --- | --- |
| Unchanged module | `f0f2984078562d0e3f239554addf0b72cb597a934cbbc07829d05696b05fa35c` |
| Unchanged header | `ffda516b8794a00a70cca1e7ae178f76dc84ce980c0e8bce7ab7ec6c38e94bc3` |
| Repaired protocol probe | `9c4b398588831cb43ed357998fe529a838a4919f96f0d2d95d9e75853f1ed71f` |
| Repaired verifier | `be33e53c5712b491eeed3e506e233106ac700b5b8996282c252a44ebc268eaea` |
| Revised supplied tests | `a46d5731dcac45194a03132582a334f609c0f74b19357b787ce28434a494c999` |
| Independent mutation tool | `a9a0117ac00d290b1ee5eccc8dd4f922bc6b7d449195c409c7d0a7c2c9583fa0` |

Final reports are in `build/scheduler-audit-v2/native-replay.json`,
`edge-integrity.log`, `independent-mutations.json` and `probe/build-manifest.json`.
The existing controlled API callback check remains applicable because its
module/header bytes are identical in v2 and v3.

Acceptance permits retaining these bounded primitives/tools for future
integration. They remain outside the production source list. It does not
accept Rules reentry, captured epoch/phase injection into production, a general
queue-mode/DMA implementation, payload pixels, elapsed work, interrupt/audio
behavior or full-game parity. Confirmed palette low-byte/borrow and incoming-M
quirks remain commented and preserved; no unknown divergence was renamed an
original-game bug.
