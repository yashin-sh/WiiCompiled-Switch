# Desktop GX capture/replay

The desktop runner renders portable Aurora command captures with the same pinned
GX/Dawn implementation as the Switch renderer. Four synthetic workloads verify
independent-process replay. An opt-in Switch build records the first frame;
a real RMCP01 capture and recognizable game pixels still need console evidence.

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

Synthetic modes are `capture`, `capture-copies`, `capture-wide` and
`capture-indexed`. `replay-check` asserts the red/blue/background workload;
`replay-copies-check` asserts the copy/clear workload. Ordinary `replay` imposes
no synthetic pixel assumptions. Invalid files are rejected before GPU creation.
The PNG reads the texture selected for presentation, including `GXCopyDisp`
before its optional EFB clear. It does not include the final surface scaling
blit or validate the Switch display backend. Its dimensions follow that source
texture, which can differ from the captured framebuffer dimensions.

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
the capture invalid. The first successful present closes and validates the
recording, then writes a temporary file and renames it only after a successful
write/close. Previous capture files are removed when recording starts.

Read these private SD files under `/switch/WiiCompiled-Switch/`:

- `fifo-capture-status.txt`: `RECORDING`, `COMPLETE` or `INVALID`, with a reason.
- `first-frame.mkwr`: written only for a complete supported first frame.

Resource pointers must resolve to a complete allocated guest RAM region before
bytes are read. Capture hooks catch errors, disable capture and report `INVALID`
without throwing into GX/HLE or changing a guarded game operation. Unknown
commands, unmapped memory, unsupported resources or exhausted budget therefore
produce an explicit diagnostic rather than a misleading replay file. Shutdown
before the first complete present reports an incomplete capture.

The budget is 8 MiB and 64 resources, including repeated snapshots and command
overhead. It is not a demonstrated RMCP01 frame budget. Capture adds RAM copies
and SD writes to an experimental build; its console timing remains unmeasured.

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
identity; it does not implement cross-frame EFB checkpoints.

Each drain snapshots all registered live ranges, including resources loaded in
previous batches. Pointers become IDs; playback allocates fresh stable storage
and applies snapshots in order. Desktop producers register memory explicitly;
the Switch producer uses the checked guest resolver. Overlapping registrations,
interior references and growth of an already registered range are refused rather
than silently splitting or reallocating a resource.

## Portable format, version 2

All integers are big-endian. Version 1 files must be regenerated.

| Field / record | Encoding |
| --- | --- |
| Header | `MKWRPL2` + NUL, two 40-byte ASCII WiiCompiled/Dawn pins, u32 width, u32 height, u32 header checksum; 100 bytes |
| Dimensions | Width 1–1920, height 1–1080; GPU readback removes row padding |
| Record prefix | u32 kind, u32 payload size, u32 payload checksum |
| Memory (1) | u32 ID followed by current bytes; IDs 1–64, stable extent |
| FIFO (2) | Command batch, pointer fields replaced by u64 IDs |
| Begin / end (3 / 4) | Empty; exactly one completed frame |
| Init (5) | Empty; exactly one `GXInit`, reset decoder and native GX defaults |
| Copy display / texture (6 / 7) | u64 destination ID (zero for display), then 59 u32 state words |
| Mapping (8) | u32 Aurora viewport policy |

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
checked. GPU tests cover direct F32 and indexed F32 positions/UVs, RGBA8 textures
and RGB5A3 EFB copies; other accepted formats need rendering coverage.

## Remaining work and validation

Little-endian display lists, nested list opcodes, guest CP array-base writes,
raw BP EFB-copy triggers, mipmapped textures and offscreen framebuffer switches
invalidate capture. Multiframe replay, arbitrary mid-game initial state, interior
resource aliases and range growth remain unsupported. The first frame may be
blank: this is a fresh-start recorder, not a screenshot trigger for a later scene.

The format is Aurora-specific, not Dolphin `.dff`. Dolphin comparison requires a
separate conversion/state model. Desktop Vulkan does not prove Switch NVK game
pixels. A valid real game capture and its desktop image remain hardware-pending.

Local checks cover ASan, fatal UBSan and LeakSanitizer; all 162 truncations and
162 single-byte corruptions of the minimal fixture; invalid lifecycle, memory,
copy state and indexed vertex bounds; and nonthrowing capture failure. Four
independent-process GPU workloads compare byte-identical PNGs and exact pixels:

- Red/blue triangles with a same-address texture update and cache invalidation.
- EFB copy, regional clear, GPU copy sampling through the optimized raw submission
  path, display copy before a full EFB clear, and destination retirement.
- A 617×341 framebuffer requiring padded GPU readback rows.
- Relocated little-endian indexed position and UV arrays.

The public desktop CI runs these synthetic workloads and sanitizer format tests;
the Switch CI compiles the opt-in recorder/SD integration without Nintendo data.
Private rendered build evidence and current CI results are recorded in the PR.

Game captures can contain copyrighted textures, palettes and vertex data.
Captures, temporary capture files, private NROs, game-derived PNGs and raw reports
remain local and excluded from publication. No CI artifact upload is configured.
