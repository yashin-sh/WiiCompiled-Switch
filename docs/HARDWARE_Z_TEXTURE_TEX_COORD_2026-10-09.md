# Depth-texture return; texture-coordinate refusal — 2026-10-09

The real-Switch trial of `0d388bb` accepts the observed
`GXSetZTexture(DISABLE, Z8, 0)` return. The fresh discovery trace records its
entry at dispatch 646694, then target `0x8060235C` at dispatch 646696 with
the subsequent cull-mode stage and later caller progression. This accepts
the executed disable tuple; it does not establish add/replace depth pixels.

The terminal translated boundary is the existing guarded `GXSetTexCoordGen2`
bridge, PAL `0x8016E37C`, at dispatch 650190 / 50.690 seconds. The captured
six-register tuple is `(0, 1, 4, 30, 0, 125)`, with LR `0x800814D8`:
coordinate 0, 2x4 generation from TEX0, texture matrix 30, normalization off
and identity post-matrix. The bridge currently admits matrix 60 (identity)
and forwards a constant `GX_IDENTITY`; matrix 30 must be audited and forwarded
before this tuple can return. The early first-hit entry for this same target
still describes its previous identity call, not the later refused tuple.

## Binding and retrieval

The exact private NRO is 74,432,568 bytes with SHA-256
`d314bab3cd80822b03f3a53a34cfe0e642506cd7773472f36bb0244d7bcc5ee6`.
Its recorded Netloader transfer completed with exit 0 at 17:43:18 UTC.
The SD candidate has also been completely read back and matched again.
Candidate code passed the separate private rendered build and all eight
published exact-HEAD checks before merging in
[PR #348](https://github.com/yashin-sh/WiiCompiled-Switch/pull/348).

USB retrieval produced 39 reports / 837,530 bytes. A second independent
USB copy verifies every report hash. Eighteen reports differ from the
normal-matrix baseline and twenty-one are retained. Both capture controllers
have changed run identifiers and report `DISABLED`; old PNGs and replay files
are not attributed to this run. The operator confirmed USB readiness without
an additional screen or fluidity observation.

## Renderer error and runtime limits

The fresh graphics report separately records a Dawn validation error:
`GX Copy Source Snapshot` is used as a texture-copy source while its usage
contains only `CopyDst | TextureBinding`. Dawn then rejects the command buffer
and its submission. The local snapshot descriptor confirms the absent
`CopySrc` bit. This is a renderer failure requiring its own correction and
validation; the later translated guard does not make the GPU path successful.
The report does not provide an exact dispatch timestamp for that error.

The sampled main-thread report, at dispatch 647715 before the terminal
boundary, retains coherent guest-fiber / OS current / OS running identities,
a structurally valid FST and six TaskThread entries. It records 6,054 FIFO
writes, drawable work, 103 `GXCopyDisp` calls and 103 successful presents,
with zero recorded presentation failures. Those presentation counters do not
count Dawn command validation failures or prove correct rendered contents.
No separate exception report is present.

Five presentation windows range from 0.192 to 9.087 Hz. They total 102
window frames over 44.828 seconds, about 2.275 Hz including loading and
stalls. This is neither steady gameplay FPS nor a controlled performance
comparison. New pixels, later menus and playability remain unproven.

Raw diagnostic reports, private NROs, captures and game data remain local.
