# First real Switch frame captured and replayed — 2026-10-08

The opt-in recorder completed a real RMCP01 first-frame capture on Switch.
The desktop runner accepted the stream, replayed it through Aurora/Dawn on
lavapipe, completed GPU readback and wrote a PNG. A second independent process
produced a byte-identical PNG.

The image is uniformly opaque black. This proves the supported first-frame
capture and desktop consumption path; it does not establish recognizable game
pixels or explain the black output of later Switch frames.

## Observed first frame

- The recorder reported `COMPLETE`, with no unsupported-operation reason.
- The validated stream contains Begin, GXInit, three FIFO batches, one
  GXCopyDisp and End, in that order.
- FIFO decoding finds one quad with four direct position vertices. It has no
  vertex colors or texture coordinates. There are no memory-resource snapshots
  for textures, palettes or indexed arrays in this frame.
- The recorded display copy has a 640×480 source and a 655×480 destination.
  The PNG is 655×480, matching the selected presentation texture. All pixels
  are RGBA `(0, 0, 0, 255)`.

The runner reads the selected display-copy texture before its optional EFB
clear. It omits the final surface scaling blit. The output dimensions therefore
need not match the frame header or the eventual display surface.

## Runtime frontier and limits

Fresh reports show translated main reached, a structurally valid FST, coherent
guest thread state, real FIFO work and 102 successful presents with zero
reported present failures. The run later stops at the existing guarded
`GXLoadTexObj` boundary for an unadmitted Mii I4 32×64 texture object. These
whole-run counters describe later execution as well as the captured first frame.

The observed first frame contains no textured scene. A later-frame checkpoint
with complete initial GX/EFB/resource state is still needed to investigate
the Mii rendering path. This result does not prove a particular relocated
texture tuple returned, Switch NVK pixel correctness, or agreement with Dolphin.
The capture format remains Aurora-specific; `.dff` conversion is unimplemented.

## Validation and evidence handling

The tested public code revision is
`786940ad6e8474f07474d3bffb903d3bda407aec`. All six public workflows / seven
jobs passed on that revision, including the four synthetic GPU workloads and
sanitizer format checks. The hardware capture passes the portable stream
validator and ordinary desktop replay, without synthetic-scene pixel assumptions.
The documentation update changes no runtime, build or test inputs.

The attributable launch, fresh reports, complete capture, inspector output,
replay logs and both PNGs are retained locally. Private NRO metadata, raw game
captures, game-derived images and diagnostic archives are excluded from public
Git and CI artifacts. See [capture/replay scope](DESKTOP_GX_REPLAY.md) and
[PR #332](https://github.com/yashin-sh/WiiCompiled-Switch/pull/332).
