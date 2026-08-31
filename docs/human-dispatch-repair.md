# Preserve the original carried-X movement bug

The first human-stage freeze is rejected, despite passing its two original
native replay captures. It decremented the actor's boost timer on a path where
the original decrements a controller-relative word. Its verifier also accepted
12 of25 independent integrity cases. The unchanged original10 files and freeze
are retained in `build/human-repair-v1/original`; the implementer's worktree and
all native captures remain untouched. This repair is not production-wired.

After the early movement gates, `$87:91D7` loads X from the controller pointer
at DP9A. `$91EB` branches to922A when the wrapped16-bit result of live-state
minus0080 has its sign bit set, skipping91ED's reload of the actor pointer.
Inside `$85:A82C`, nonzero full16-bit actor Z takes A850/AAE8 beforeA91F reloads
X. Thus AB06/AB13 reads/writes **controller pointer+72**, leaving the actor's
boost timer unchanged. This can address beyond the selected64-byte record.
The original restores DP C6 before subtracting: zero skips the store; otherwise
the wrapped subtraction result is clamped to zero if bit15 is set. This is
not a borrow-based clamp:2 minusFFFF becomes3. The port must preserve it.

The original two captures contain52 such airborne pointer paths (11left,
41right). Their affected words are zero, so the original projection alone did
not distinguish the wrong target. The independent auditor first made a
clearly labeled C-only copy of natural entry2844 with actor boost5 and the
controller-relative word7. With delta2, original source requires5/5; the
rejected module returned3/7. No native memory was modified in that check.

The auditor then obtained a fresh natural witness using ordinary L held with
the existing X jump. The original ROM/private Mesen run used no RAM, register,
ROM or ownership injection. Its manifest is
`completion-auditor/build/human-audit-v1/native-airborne-lx-v1/manifest.json`,
SHA2561ff32ce8bbdcca41a4323dabeecb131b8a6a8bcc66ebccd5f7fbddbf54082406.
At motion entries2844/2889/2914 (court267/272/275), the original retains actor
boost5 while the rejected C module produces3. The exact PC and access evidence
is documented next to the repaired code in `src/nba_human_dispatch.c`.

The repaired semantic input includes the referenced `controller_word_72`.
Only the source-defined airborne carried-X route selects it for decay; the
existing velocity helper performs the original arithmetic, without changing
that global helper. Actor-target paths still decay actor boost. Blocked paths
do not consult the direction vector. The probe loads only native entry memory,
writes the resulting referenced word, and separately reports it. The verifier
compares that scalar with the native exit as well as every existing actor,
controller and context word. The extra projection also covers a fifth-pad
address beyond the five-record block; fifth-pad gameplay remains unobserved.

The verifier repair requires exact integer/boolean types, sparse ranges,
runner frame bounds, executed command, route environment, source paths,
artifacts, strict typed event rows, ordered clocks and completion fields.
It reuses the independently accepted controller isolation verifier, which
compares actual settings/home/save attestations before calling a helper on a
copy. Extra or malformed C output and empty stage coverage remain rejected.
Both verifier source hashes are included in successful reports.

Fresh `/W4 /WX` builds and owner checks in `build/human-repair-v2` pass:

- Original left337904 and right367653 compared values (705557 total). The
  original703914 comparisons remain;1643 referenced-word checks are added.
  All29 recorded frame crossings remain represented.
- Natural L+X witness59093 values:1690 input gates,137 B prefixes,137 movement
  calls, including the three nonzero actor-boost cases and both frame crossings.
- All12 independent controlled pointer/arithmetic cases and14 additional
  controlled route/word cases. These are labeled C-only source contracts.
- The independent25-case verifier integrity suite.

Independent re-review accepted the repair: see
`completion-human-dispatch-repair-independent-audit.md`. The auditor freshly
built the probe, rehashed all27 frozen files and40 link objects, compared all
764650 native values, and passed the25 integrity,12 alias and14 quirk cases.
It separately verified the fifth-pad referenced-word case. The exact reviewed
version of this owner note remains in
`build/human-repair-v2/reviewed-owner-repair.md`; only this acceptance note was
added afterward. The rejected v1 audit and all its failures remain intact.
This module remains outside
`nba95_sources.txt`; normal human gameplay stays disabled. B children, other
buttons, physical actor commit, full caller ordering and natural whole-game
parity remain separate work. Preserving this bug does not certify those paths.
