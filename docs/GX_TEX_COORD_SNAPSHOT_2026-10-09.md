# Observed texture matrix and copy-source usage correction — 2026-10-09

The [depth-texture trial](HARDWARE_Z_TEXTURE_TEX_COORD_2026-10-09.md) exposes
two independent failures: the Gen2 bridge refuses matrix 30, and Dawn rejects
an exact copy from `GX Copy Source Snapshot` without `CopySrc` usage.

The Gen2 bridge now admits `(0,1,4,30,0,125)` alongside the existing identity
tuples on coordinates 0..7. It forwards the captured matrix word to the
pinned writer instead of always selecting identity. Other matrices,
coordinates with matrix 30, types, sources, normalization and post-matrices
remain guarded. The pinned wrapper forwards matrix as `u32`; Aurora writes
the coordinate's six-bit native matrix-index shadow and its CP/XF registers.
This bridge adds no guest-memory mirror or matrix loading.

Both headless and rendered contracts preserve the complete CPU context and
guest memory, check native forwarding and refuse neighboring/wide matrix
words and independent tuple variations before output. The actual pinned Gen2
writer, matrix-index helper, register macros and complete native state struct
execute in 36 packet cases. Independent expected bytes cover both matrix-index
banks and the XF/CP stream; complete state canaries permit only the selected
matrix-index field and native `bpSent` update.

The checked Aurora build mirror adds `CopySrc` only to the snapshot texture's
existing `CopyDst | TextureBinding` usage. The preparation script verifies the
whole pinned renderer source hash and exact descriptor before replacement.
Original dependency files and local integration patches remain untouched.
Switch rendered builds and desktop replay use this same preparation path.

The GPU regression uses an alpha-bearing EFB, a neutral copy filter, native
coordinates and matching RGBA8 dimensions to select the exact snapshot-copy
path. The earlier RGB5A3/filtered test takes a shader path and did not exercise
this failure. The new regression fails GPU readback validation with the old
descriptor. With the correction, separate capture/replay processes produce
identical PNGs and pass the sampled-red and cleared-background pixel oracles.
The complete existing desktop suite also passes, including converted copies,
indexed vertices, wide readback, multi-frame updates and partial-tile formats.

Local contracts and GPU regression pass. Every published exact-HEAD CI check,
SDK syntax, the separate private rendered build and provider/retention audits
are still required before merge/deployment. The new Gen2 return, absence of
Dawn errors on Switch and later game pixels require a fresh console trial.
