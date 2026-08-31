# Accepted human pass pose component

The AF1D pose, B649/B832 attachment and AF30 commit component is independently
accepted with revision2 of its verifier. The original checkpoint and verifier
remain preserved, together with the original audit rejection. Revision1 did
not enforce the binary arithmetic domain: altered decimal-mode input flags
could pass validation. Revision2 requires D=0 as witnessed at all actual native
entries. It changes no C arithmetic or captured state.

Root rechecked 206 original and101 revision2 referenced identities, plus each
freeze itself; copied19 source/audit/tool files byte-for-byte; rebuilt the C
probe with MSVC /W4 /WX; and reproduced all262650 native value comparisons,
82 local rejection cases and the unchanged13 independent rejection cases.
The independent source review additionally checks889 literal source cases.
The root probe SHA256 is
`8992c7ad3eae247e6eff38ca3d0af1b4f71c0d6f66a48ddfa4dd8b46031e016f`.
Root copy and verification records are under `build/pose-integration-v1`.

The component preserves literal phase selection, signed-byte/midpoint behavior,
wrapped comparisons and the existing original word writes. Synthetic extreme
inputs remain source-contract coverage, not naturally witnessed gameplay.
The accepted native slice stops before AF4D stack restoration. Catch geometry,
the epilogue and the full human control sequence remain separate checkpoints.
No production manifest or human-enable gate changes in this integration.
