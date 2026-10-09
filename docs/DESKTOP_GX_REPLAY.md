# Desktop GX capture/replay

The desktop runner renders portable Aurora command captures with the same pinned
GX/Dawn implementation as the Switch renderer. Seven synthetic workloads verify
independent-process replay. An opt-in Switch build records a completed prefix
from GXInit, including successive frames. The [latest console trial](HARDWARE_NONBLACK_REPLAY_2026-10-09.md)
saves and successfully replays 90 complete frames within the capture budget.
A derived four-frame prefix reproduces the recognizable Wiimote safety page;
frame 90 remains black on both platforms. Frame-4 RGB closely agrees with the
Switch selected copy, with small differences; the comparison is not bit-exact.
Later rendering and playability remain unproven. The [preceding trial](HARDWARE_CAPTURE_CONTROL_2026-10-09.md)
exhausted its budget after four frames with only one saved, before deduplication
and first-nonblack retention were corrected.

## Desktop build and tests

Requires Linux, CMake, a C++20 compiler, Python 3, zlib development headers and
Vulkan. Tests automatically select the installed Mesa lavapipe manifest, unless
`VK_DRIVER_FILES` or `VK_ICD_FILENAMES` is supplied.

```sh
MKW_JOBS=4 bash scripts/build-desktop-gx-replay.sh
bash scripts/test-desktop-gx-replay.sh
.deps/desktop-gx-replay/mkw-gx-replay replay /tmp/first-frame.mkwr /tmp/frame.png
```

The builder prepares its own pinned Dawn checkout under ignored `.deps/`.
`MKW_REPLAY_DAWN_ROOT` selects an already prepared checkout;
`MKW_REPLAY_BUILD_ROOT` selects the build/output directory. Dependencies and
instrumentation sites are checked against their pins. Instrumentation modifies
a build mirror, preserving the original WiiCompiled checkout and local patches.

Synthetic modes are `capture`, `capture-copies`, `capture-wide`,
`capture-indexed`, `capture-sequence`, `capture-i4` and `capture-rgb5a3`. `replay-sequence-check` verifies the
second-frame color oracle and the retained GX state. `replay-i4-check` checks
white/black intensity pixels; ordinary `replay-check` checks direct RGB5A3 colors. `replay-check` asserts the red/blue/background workload;
`replay-copies-check` asserts the copy/clear workload. Ordinary `replay` imposes
no synthetic pixel assumptions. Invalid files are rejected before GPU creation.
The runner applies the shared selected-XFB presentation pass after Aurora renders
each frame. It samples the display copy preserved before any EFB clear, with
opaque display alpha as in pinned Aurora. Desktop output dimensions follow that
copy, which can differ from the capture header and the scaled Switch surface.
This tests the shared pass on desktop, not physical Switch scanout.

## Opt-in Switch capture

Prepare the local product as described in [BUILD.md](BUILD.md), then run:

```sh
MKW_RENDERED_FIFO_CAPTURE=ON MKW_DISCOVERY_SCAN_MODE=ON MKW_JOBS=4 \
  bash scripts/build-local-rendered-fast-track.sh
```

This produces private `WiiCompiled-Switch-local-rendered-fifo-capture.nro`, with
an explicit capture title. The existing normal and Discovery outputs keep their
names and have capture disabled by default. Existing HLE argument guards remain
in force. Launch in hbmenu application/title-override mode with full memory.

The capture arms before the renderer opens its first frame. It requires exactly
one `GXInit`; FIFO work before initialization or another initialization makes
the capture invalid. Each successful present marks a completed frame and
validates its prefix. The first file is retained; the latest prefix is saved on
frame 1, every 30 presents and at a diagnosed unsupported dispatch, capture failure or shutdown.
Saving uses the complete-write/backup/rollback path shared with PNGs. Previous
capture files are retired when recording starts.

Read these private SD files under `/switch/WiiCompiled-Switch/`:

- `fifo-capture-status.txt`: `RECORDING`, `COMPLETE`, `INVALID` or `DISABLED`, with a reason.
- `first-frame.mkwr`: retained complete first frame.
- `latest-frames.mkwr`: state-preserving prefix through `saved_frame`; the
  current partially recorded frame is excluded. The status gives the unique run
  ID, completed/saved frame counts and save size.

Resource pointers must resolve to a complete allocated guest RAM region before
bytes are read. Capture hooks catch errors, disable capture and report `INVALID`
without throwing into GX/HLE or changing a guarded game operation. Unknown
commands, unmapped memory, unsupported resources or exhausted budget therefore
produce an explicit diagnostic rather than a misleading replay file. Shutdown
before the first complete present reports an incomplete capture.

The budget is 8 MiB and 64 resources for the entire sequence, including changed
snapshots and command overhead. Unchanged bytes reuse their last serialized
snapshot; they do not consume another resource-sized record. It is not a demonstrated RMCP01 sequence budget.
After an invalidation, a labelled earlier complete prefix can remain on SD;
`INVALID` never establishes complete coverage of the current run. Capture adds RAM copies
and SD writes to an experimental build; its console timing remains unmeasured.

Create `switch/WiiCompiled-Switch/render-captures-disabled.flag` on SD before
launching to disable both FIFO and GPU image capture at startup in the same NRO.
Both controllers report `DISABLED`, leave earlier files intact and perform no
snapshot/readback work. Remove the marker before the next diagnostic launch to
restore capture. The marker is not polled to change an active run. Normal builds
already default to capture OFF. See [image/control details](SWITCH_FRAME_DUMP.md).

## Capture boundary and lifetime

Hooks observe the common FIFO drain, directly consumed big-endian display lists,
optimized `submit_raw_draw` calls after their pending state drains,
`GXInit`, viewport-policy changes and native `GXCopyDisp` / `GXCopyTex` after
their preceding FIFO work drains. Successful direct raw submissions become complete draw packets in the portable
stream. The native setters and guest HLE/burst paths
meet at these Aurora consumption boundaries; raw guest input bytes are not
recorded separately.

Each copy event includes the live copy configuration, filter, clear state and
format. Texture-copy destinations share portable resource IDs with later texture
loads and FIFO-ordered destination destruction. Replay reproduces the native GPU
copy and its clear before later draws sample it. RAM snapshots do not substitute
for a GPU copy. This preserves the pinned implementation's first-frame cache
identity. Successive frames replay from initialization, retaining GPU resources,
GX state and pointer identities; this is not an arbitrary mid-game checkpoint.

Each drain compares all registered live ranges, including resources loaded in
previous batches, and emits a new snapshot only when bytes differ. Pointers become IDs; playback allocates fresh stable storage
and applies snapshots in order. Desktop producers register memory explicitly;
the Switch producer uses the checked guest resolver. Overlapping registrations,
interior references and growth of an already registered range are refused rather
than silently splitting or reallocating a resource.

## Portable format, versions 2 and 3

All integers are big-endian. Version 1 files must be regenerated. Version 2
single-frame files remain readable; sequence checkpoints use version 3.

| Field / record | Encoding |
| --- | --- |
| Header | `MKWRPL2` or `MKWRPL3` + NUL, two 40-byte ASCII WiiCompiled/Dawn pins, u32 width, u32 height, u32 header checksum; 100 bytes |
| Dimensions | Width 1–1920, height 1–1080; GPU readback removes row padding |
| Record prefix | u32 kind, u32 payload size, u32 payload checksum |
| Memory (1) | u32 ID followed by current bytes; IDs 1–64, stable extent |
| FIFO (2) | Command batch, pointer fields replaced by u64 IDs |
| Begin / end (3 / 4) | Empty; open a GPU frame / close the final frame and file |
| Init (5) | Empty; exactly one `GXInit`, reset decoder and native GX defaults |
| Copy display / texture (6 / 7) | u64 destination ID (zero for display), then 59 u32 state words |
| Mapping (8) | u32 Aurora viewport policy |
| Frame (9, v3 only) | Empty; complete the current frame, retain state and require a new Begin |

The checksum starts at `2166136261 XOR kind` and applies
`value = (value XOR byte) * 16777619` modulo 2³². Header checksum uses kind 5
and the preceding 96 bytes. Checksums detect corruption, not authenticity.
Lengths, lifecycle, resources and command boundaries are validated before any
GPU callback. Unknown records, trailing data and incomplete frames are refused.

Copy state words follow this layout (float fields contain IEEE-754 binary32 bits):

| Words | Values |
| --- | --- |
| 0–9 | Clear, source x/y/width/height, destination width/height, texture format, half scale, source in render coordinates |
| 10–16 | Viewport policy, clamp, field mode, gamma, Y scale float, AA filter enable, vertical filter enable |
| 17–40 | Twelve x/y sample pairs |
| 41–47 | Seven vertical filter coefficients |
| 48–54 | Clear RGBA floats, clear depth, pixel format, Z format |
| 55–58 | Depth update, color update, alpha update, destination alpha (UINT32_MAX disables) |

FIFO parsing supports scalar position/normal/UV formats, packed colors, matrix
indices, indexed attributes with checked array extents/strides and indexed XF
loads. Aurora array/texture/TLUT metadata, viewport/scissor/debug operations,
cache invalidation and copy-destination destruction are relocated. Non-mipmapped
tiled I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8, C4, C8, C14X2 and CMPR sizes are
checked. GPU tests cover direct F32 and indexed F32 positions/UVs, RGBA8, I4 and RGB5A3
textures with partial tiles, and RGB5A3 EFB copies; other accepted formats need
rendering coverage.

## Remaining work and validation

Little-endian display lists, nested list opcodes, guest CP array-base writes,
raw BP EFB-copy triggers, mipmapped textures and offscreen framebuffer switches
invalidate capture. Arbitrary mid-game initial state, interior resource aliases
and range growth remain unsupported. The first frame may be blank; replaying a
later frame requires the prefix from initialization, not skipping earlier state.

The format is Aurora-specific, not Dolphin `.dff`. Dolphin comparison requires a
separate conversion/state model. Desktop Vulkan does not prove Switch NVK game
pixels. The first real Switch capture validates producer-to-desktop replay,
but contains no textured scene. The new later-frame sequence capture and shared
presentation pass still need console validation; game-image correctness is pending.

Local checks cover ASan, fatal UBSan and LeakSanitizer; all 162 truncations and
162 single-byte corruptions of the minimal fixture; invalid lifecycle, memory,
copy state and indexed vertex bounds; and nonthrowing capture failure, v3 frame boundaries and immutable completed
prefixes. Seven independent-process GPU workloads compare byte-identical PNGs and exact pixels:

- Red/blue triangles with a same-address texture update and cache invalidation.
- EFB copy, regional clear, GPU copy sampling through the optimized raw submission
  path, display copy before a full EFB clear, and destination retirement.
- A 617×341 framebuffer requiring padded GPU readback rows.
- Relocated little-endian indexed position and UV arrays.
- Partial-tile I4 white/black and direct RGB5A3 red/blue textures with data updates.
- Two completed frames retaining slot/VAT/projection/sampler state and
  same-address texture updates, followed by an excluded partial third frame.

The public desktop CI runs these synthetic workloads and sanitizer format tests;
the Switch CI compiles the opt-in recorder/SD integration without Nintendo data.
Public validation evidence and current CI results are recorded in the PR;
private rendered build and deployment details remain local.

Game captures can contain copyrighted textures, palettes and vertex data.
Captures, temporary capture files, private NROs, game-derived PNGs and raw reports
remain local and excluded from publication. No CI artifact upload is configured.

The synthetic suite also runs `capture-lyt-quads` / `replay-lyt-quads-check`
and `capture-lyt-colors` / `replay-lyt-colors-check`. These scenes link the
production Switch DrawQuad bridge, real checked Memory slice and Aurora native
setters/display-list decoder, with synthetic guest backing and frame ownership.
Textured and vertex-colored quads must cover eight additional interior pixel
samples and preserve the background. Separate capture/replay processes must
produce identical PNGs. Alpha packet semantics are covered by
`bash scripts/test-lyt-draw-quad.sh`; selected-XFB output has opaque alpha.
