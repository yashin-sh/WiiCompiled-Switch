# Hardware result — Discovery scan / GXSetPixelFmt frontier (2026-10-02)

The rendered Discovery NRO built after the PAL GXInvalidateTexAll bridge
progresses through that texture-cache invalidation boundary and reaches the
next exact unsupported GX call.

## Durable blocker

```text
kind                  : DIRECT
target                : 0x80172888
lr                    : 0x80240ecc
r3                    : 0x00000001
r4                    : 0x00000000
r5                    : 0x00000001
fast-track stage      : RMCP01_GX_INVALIDATE_TEX_ALL
```

The runtime discovery log records this as unique first-hit target 1,239 at
dispatch 604,984.

Pinned WiiCompiled at
`a135beb201042b20f390c6695ca6b26768820fb4` maps PAL `0x80172888` to
`GXSetPixelFmt` and forwards `r3/r4` directly as
`GXPixelFmt/GXZFmt16` to Aurora:

```cpp
GXSetPixelFmt(static_cast<GXPixelFmt>(pf), static_cast<GXZFmt16>(zf));
```

The observed hardware tuple is therefore `pf=1, zf=0`.

## Run health

The same run still records:

- PAL main reached;
- structurally valid FST at `0x97DC0000`;
- 19 successful DVD reads in the supplied snapshot;
- successful `Font.szs` decode (3,153,052 bytes produced);
- 1,555 RMCP01 FIFO writes;
- 99 `GXCopyDisp` calls;
- 99 successful presents;
- zero present failures.

This proves progression beyond the merged `GXInvalidateTexAll` bridge rather
than a regression in the renderer, DVD/FST, decompression or scheduler path.

## Port decision

Map only PAL `GXSetPixelFmt (0x80172888)` to the pinned two-argument Aurora
call. Do not pre-port the neighboring `GXSetDither (0x80172930)` or
`GXSetDstAlpha (0x8017295C)`; they remain future hardware-defined frontiers.
