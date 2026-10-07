# Desktop GX capture/replay prototype

This Nintendo-data-free prototype records one synthetic Aurora frame, restores
its resources in a separate desktop process and renders a PNG with the pinned
Aurora GX / Dawn Vulkan implementation. It is a diagnostic foundation for
future game captures. It does not yet capture RMCP01 on Switch.

## Build and run

Linux requires a C++20 compiler, CMake 3.20+, Python 3, Git, zlib development
headers and a Vulkan implementation. Mesa lavapipe can run without a display
or physical GPU. Initialize the WiiCompiled submodule first:

```sh
git submodule update --init --recursive
MKW_JOBS=4 bash scripts/build-desktop-gx-replay.sh
bash scripts/test-desktop-gx-replay.sh
```

The build helper clones the exact public Dawn dependency when absent; it refuses
to change an existing checkout at another revision. An already prepared pinned
Dawn tree can be supplied with `MKW_REPLAY_DAWN_ROOT`. The default build and all
outputs live under ignored `.deps/desktop-gx-replay/`; override the build directory
with `MKW_REPLAY_BUILD_ROOT`. Extra arguments go to CMake, including
`FETCHCONTENT_SOURCE_DIR_FMT`, `FETCHCONTENT_SOURCE_DIR_XXHASH` and
`FETCHCONTENT_SOURCE_DIR_TRACY` for existing public dependency sources.

The test helper discovers the installed `lvp_icd*.json` manifest rather than
assuming an architecture suffix. To select another driver, or resolve multiple
installed manifests, set `VK_DRIVER_FILES` or `VK_ICD_FILENAMES` explicitly.

Manual commands:

```sh
.deps/desktop-gx-replay/mkw-gx-replay capture /tmp/scene.mkwr /tmp/original.png
.deps/desktop-gx-replay/mkw-gx-replay replay /tmp/scene.mkwr /tmp/replayed.png
```

`replay-check` adds the synthetic scene's exact red, blue and background pixel
oracles. Ordinary `replay` renders any supported capture without assuming those
colors. GPU errors, failed readback, invalid captures and failed oracles return
nonzero. Output is an offscreen 256×256 RGBA8 PNG; no window or Switch transfer
is needed.

## Capture boundary and order

The build mirror instruments Aurora's `fifo::drain()` immediately before its
command processor consumes a batch. Native GX setters and the raw big-endian
FIFO producer meet at this boundary. The original WiiCompiled checkout and its
local Switch patch are preserved. The mirror reuses the checked display-list
writer and float-scalar preparation used by the public Switch probe.

The first frame begins after recording the initialization stream. Playback
executes the same pinned `GXInit` to establish non-FIFO defaults, discards its
queued bytes and restores the captured initialization batches. This is a
fresh-start protocol; arbitrary mid-game state checkpoints are unsupported.

All live resource ranges must be explicitly registered by the producer. Each
drain snapshots those ranges before command consumption. Resource pointers in
Aurora texture, TLUT and array commands become stable IDs in the file. Playback
allocates fresh storage, applies each snapshot in order and replaces IDs with
local pointers. Repeated snapshots retain a stable allocation, including when a
previous batch already loaded the resource. Interior pointers, unknown ranges
and ranges smaller than the metadata's required storage are refused.

The synthetic scene draws a red textured triangle using native GX. It then
changes the same texture buffer to blue, invalidates the texture cache, loads a new texture-object identity and
draws a second triangle with raw FIFO vertices. Two independent processes must
produce identical PNGs and pass exact red/blue/background pixel checks. This
checks native/raw producer ordering, resource relocation, tiled texture bytes,
same-address updates, rendering and GPU readback. It does not exercise the
pinned guest `HleFifoWrite` decoder or its optimized burst path yet.

Recording buffers events in RAM and writes the file only after a completed
frame and successful pixel validation. The file limit is 8 MiB, with at most
64 registered resource ranges. No file I/O occurs for individual FIFO writes.
This bound is a prototype limit, not a demonstrated game-frame budget.

## Portable format, version 1

All integers are big-endian. Files contain:

| Field | Encoding |
| --- | --- |
| Magic/version | Eight bytes: `MKWRPL1` followed by NUL |
| WiiCompiled and Dawn pins | Two 40-byte ASCII commit IDs |
| Width and height | Two u32 values; both must be 256 |
| Records | u32 kind, u32 payload size, u32 checksum, payload |
| Memory record (kind 1) | u32 resource ID followed by its current bytes |
| FIFO record (kind 2) | Command batch with pointer fields replaced by u64 IDs |
| Begin / end (kinds 3 / 4) | Empty payload; exactly one completed frame |

The record checksum starts at `2166136261 XOR kind`, then applies
`value = (value XOR byte) * 16777619` for every payload byte, modulo 2³².
It detects accidental corruption; it is not an authenticity mechanism. The
reader validates the complete file, pins, record lengths, resource references,
command boundaries and frame lifecycle before creating a GPU device. Trailing
records and incomplete captures are refused.

Supported FIFO commands include BP/CP/XF writes, cache invalidation, selected
Aurora resource/viewport/scissor/debug commands, and complete quads, triangles,
strips and fans with direct XYZ/F32 positions, optional RGBA8 colors and ST/F32
UVs. Indexed vertices, matrix-index attributes, normals and other vertex
formats are refused. Non-mipmapped tiled I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8,
C4, C8, C14X2 and CMPR storage sizes are checked; the initial GPU scene exercises
RGBA8 only. Array and palette metadata can be relocated, but indexed drawing
and palette rendering are not yet integration-tested.

## Remaining game-capture work

`GXCopyDisp`, `GXCopyTex` and other GX operations can update live Aurora state
or schedule graphics work directly rather than solely emitting FIFO bytes.
The prototype does not install hooks for those operations, and does not claim
that the common drain alone captures all game rendering. BP copy commands,
destroy-copy commands, indexed XF and nested display-list calls are refused.

Before enabling capture on Switch, add ordered events for direct operations,
copy/cache lifetimes and frame boundaries; register proven guest memory ranges
at consumption; cover guest HLE and optimized bursts; and validate the initial
state plus cross-frame EFB dependencies. Unknown operations must invalidate the
capture explicitly. Keep normal hardware bring-up and its guarded boundaries
unchanged until an opt-in capture build passes the existing validation ladder.

This format contains Aurora extension commands and is not Dolphin `.dff`.
Dolphin export would require a separate conversion and state/resource model.
Desktop Vulkan results do not validate Switch NVK behavior or game pixels.

## Validation and publication boundary

Local Linux/lavapipe validation on 2026-10-07 passes with GCC 16 for the full
desktop build and Clang 22 for ASan, fatal UBSan and LeakSanitizer format tests.
The format suite refuses all 146 truncations and 146 single-byte corruptions
of its minimal fixture, trailing data, invalid lifecycle, unsupported commands,
unknown/undersized memory, invalid texture metadata and the 8 MiB size limit.
It also verifies independent pointer relocation and ordered same-address updates.

The synthetic `.mkwr` is 2,484 bytes. Independent capture and replay each produce
a 1,328-byte PNG and pass exact pixels `(64,128) = (255,0,0,255)`,
`(192,128) = (0,0,255,255)` and `(8,8) = (64,64,64,255)`. Both local PNGs have
SHA-256 `40c2799ee1659dd7df694fe116c06145759ecfc7793021d28090abff4a013614`.
That digest records this environment's result; CI compares its own two PNGs
rather than assuming compressed PNG bytes match across zlib versions.

The same-address mutation needs `GXInvalidateTexAll` before reloading the blue
texture. Without that operation, the native reference scene itself retains the
cached red texture. The recorded invalidation is therefore part of the workload,
and the test does not treat a texture reload alone as an invalidation guarantee.

The `desktop-gx-replay` CI workflow builds only public code and creates only the
synthetic scene. It runs format regressions under ASan/UBSan, then renders
capture and replay in separate processes with lavapipe and compares PNG bytes.
Local format tests additionally run with LeakSanitizer enabled.

Validation results for this change are recorded in the associated pull request.
Only synthetic sources and public build tooling may be published. Game captures
can embed copyrighted textures, palettes and vertex data: `.mkwr` and `.dff`
are excluded from Git, and raw game captures, private NROs and game-derived PNGs
must remain local. No CI artifact upload is configured.
