# Controller verifier integrity repair

The independent audit found attestation and empty-coverage failures in the
first controller verifier, despite the actual frozen native comparisons
passing. The owner repaired the verifier in this integration worktree;
the implementer's19-file controller source freeze and all native artifacts
remain unchanged. This is not controller production integration yet.

The verifier now requires the actual trace/completion/executed scripts/settings
artifacts and all five source identities, ties source paths to the files they
name, rejects boolean/float schema and count substitutions, verifies private
paths and fixed capture settings, and compares recorded persisted settings and
save hashes with actual observations. The helper receives a copy of isolation
metadata so it cannot overwrite a bad recorded hash during verification.
Declared selection/frame environment must agree with the manifest; observed
Player Setup selection and completion snapshot count must agree too.

The first immutable left capture predates the configurable `court_frames`
manifest field. Its executed Lua completes at literal court==400. Compatibility
is limited to the exact recorded runner/Lua SHA256 pair, both rehashed from
their named files; unknown/newer manifests with a missing limit are rejected.
This is an explicit older-format contract, not a default for missing evidence.

The C response is parsed completely. Input-mode probes legitimately print one
exact original-ROM loader line before JSON; only that line at that location is
accepted. Arbitrary diagnostics, extra JSON and unframed output are rejected.
Duplicate/empty mode selections and a requested subset with no compared words
are rejected. Successful reports explicitly list modes with no observations,
rather than suggesting full branch coverage.

Owner reports `build/controller-owner-v3-*.json` preserve all four comparisons:
left25292words, neutral716, right121916, right-live-pass122096, total270020.
All expected values remain derived from unchanged raw exits. The independent
34-case mutation suite passes in `build/controller-integrity-owner-v3.json`.
Native process/capture bytes were never changed. Earlier failing repair runs
are retained: they exposed the older fixed400 format and legitimate input ROM
loader output before those exact contracts were added.

The second independent review exposed11 further route/command mutations that
the verifier accepted. Revision4 requires the exact five source keys and native
command (private executable, test runner, timeout, original ROM and executed
Lua), fixed team-variant0/no-pause environment and the runner's400..2000 court
limit. Live-pass metadata and environment follow the executed immutable runner
revision; older runners cannot claim that route. Unknown runner contracts are
rejected for review rather than silently inheriting a schema interpretation.
All11 additional rejection cases and the original34 pass. The four unchanged
captures still compare270020words in `build/controller-owner-v4-*.json`.

Independent re-review passed; see `completion-controller-independent-audit.md`.
Owner production integration is documented separately in
`controller-integration-checkpoint.md`. Normal human gameplay, full gameplay
parity and the complete game are not accepted by these verifier-only results.
