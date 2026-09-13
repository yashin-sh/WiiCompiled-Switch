# Upstream GX audit — 2026-09-13

## Purpose

This note records WiiCompiled/Aurora issues that can affect the Nintendo Switch port before a native graphics backend is complete. The goal is to keep shared GX decoder/runtime faults separate from Horizon/libnx/backend faults during M3.

The Switch port is pinned to WiiCompiled commit:

`a135beb201042b20f390c6695ca6b26768820fb4`

The audit also inspected upstream commit `209405dfb72c73d6bc26f5214bf4dfe0f8baae2f`. For the main affected file, `aurora-main/lib/gx/command_processor.cpp`, both refs resolve to the same blob (`5f69705e96fce50d69235c106dd7bf4816281724`). The four findings below therefore apply to the pinned source consumed by this project, not only to a future upstream rebase.

## Findings that block trustworthy M3 debugging

### #109 — Release-safe FIFO bounds checking

Aurora's `CHECK(...)` macro is compiled out under `NDEBUG`. Some FIFO paths already use explicit range validation, but other parsing paths still rely on `CHECK()` immediately before reading from the packet buffer.

**Risk on Switch:** an optimized `.nro` can read outside the supplied display-list/FIFO range and fail as a Horizon exception, corrupted decoder state, or misleading GPU/backend crash.

**Rule for the port:** safety-critical packet validation must remain active in Release builds. Debug assertions can supplement validation but cannot be the only guard.

### #110 — Draw merges must preserve `GXVtxFmt`

The GX draw merge fast path can merge a new draw into the previous `DrawData` without proving that both commands use the same `GXVtxFmt`. Vertex size and pipeline resolution are format-dependent, while `DrawData` currently does not retain the format needed for that compatibility check.

**Risk on Switch:** vertex bytes from one layout can be consumed by a draw/pipeline representing another layout, producing malformed geometry, normals, UVs, or backend-visible invalid data.

**Rule for the port:** draw-call optimization is allowed only after proving vertex-layout compatibility. A regression test must cover `GX_VTXFMT0` followed by a different format.

### #111 — Degenerate `GX_LINESTRIP` counts

The line-strip instance count is derived from `vtxCount - 1`. Counts below two are not rejected before this arithmetic.

**Risk on Switch:** zero vertices can underflow into a huge unsigned instance count; one vertex is a degenerate draw. Either case can reach the backend with nonsensical work.

**Rule for the port:** reject or skip primitives that do not meet their minimum legal/meaningful vertex count before index or instance-count generation.

### #112 — Unsupported indexed XF loads must not disappear in Release

When an indexed XF load cannot be handled by `copy_xf_data(...)`, the current diagnostic is debug-only. In Release, the state update may be silently discarded.

**Risk on Switch:** transforms/matrix-related state can be wrong while the renderer continues, making a shared GX decoder limitation look like a Switch graphics-backend bug.

**Rule for the port:** unsupported state-changing GX/XF commands must either be implemented, fail explicitly, or produce a release-visible rate-limited diagnostic. They must not disappear silently.

## Priority and sequencing

These fixes are not a reason to stop M2 runtime/bootstrap work. They become gating items when M3 starts consuming real GX traffic and rendering a first frame.

Recommended order:

1. #109 — bounds safety first, because every later GX diagnostic depends on trustworthy parsing.
2. #110 — draw compatibility, to avoid subtle geometry corruption.
3. #111 — primitive-count validation, a small and deterministic hardening fix.
4. #112 — XF visibility/coverage, so missing GPU state cannot masquerade as a backend defect.

## Non-blocking upstream debt noted during the same audit

The audit also observed broader debt that should remain visible but does not need a dedicated Switch blocker yet:

- runtime-native registration analysis in the translator still uses regex/source scanning and upstream itself describes the approach as fragile;
- Aurora still contains GX fatal stubs for functions that have no native implementation bound yet;
- some PC-specific memory guarding, audio, media-session, windowing and input code should not be copied to Horizon and should instead be replaced by Switch-native adapters.

Those items should be revisited when their execution paths become reachable on hardware or when the upstream pin is intentionally rebased.

## Validation policy for M3

A first clear frame or first game frame is not considered trustworthy until:

- the FIFO decoder is memory-safe in optimized builds;
- draw merges have layout-compatible regression coverage;
- primitive-count arithmetic cannot underflow;
- unsupported indexed XF state is observable;
- the Switch backend and the shared GX decoder have separate diagnostics.

Tracking issues: #109, #110, #111, #112.
