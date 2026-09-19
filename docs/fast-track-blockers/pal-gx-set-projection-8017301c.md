# PAL GXSetProjection — 0x8017301C

Tracking: #117, #162

## Hardware blocker

The first hardware run after #192 advances to:

```text
kind   : DIRECT
target : 0x8017301C
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_DRAW_DONE
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, this is
`GX__SetProjection_8017301c`.

## Pinned semantics

The native override:

1. maps 64 bytes of guest matrix memory;
2. converts 16 big-endian float32 values to host floats;
3. calls Aurora `GXSetProjection(matrix, projectionType)`;
4. updates the pinned runtime's internal projection-vector cache.

The current hardware blocker requires the first three behaviors directly.
If RMCP01 later reaches `GXGetProjectionv`, the projection-vector cache becomes
a separate observable boundary and must be mirrored then rather than adding the
whole transform family speculatively.

## Switch implementation

The rendered fast-track bridge reads the guest matrix through the existing
endian-aware Memory API, reconstructs the host float matrix, then forwards it to
Aurora GX. Headless/synthetic builds keep the HLE boundary without adding an
Aurora dependency.

Invalid guest matrix ranges abort with a durable diagnostic rather than
fabricating success.

The fast-track diagnostics also gain explicit `TaskThread::run hits` and
`GXSetProjection hits` counters so the next hardware run can prove both
boundaries independently.
