# Hardware result — Discovery scan / GXSetCoPlanar frontier (2026-10-02)

The rendered Discovery NRO built on `bb4f873` after the PAL GXSetPixelFmt
bridge crosses that boundary and reaches the next exact unsupported GX call.
The NRO was transferred and launched with nxlink; diagnostics were retrieved
from the Switch SD through USB/MTP.

## Durable blocker

```text
kind                  : DIRECT
target                : 0x8016f3e0
lr                    : 0x80240ecc
r3                    : 0x00000000
fast-track stage      : RMCP01_GX_SET_CURRENT_MTX
```

The first-hit discovery log records GXSetPixelFmt at dispatch 609,166 with
`r3=1/r4=0`, then the distinct GXSetCoPlanar target as unique target 1,240
at dispatch 609,175. The intervening GXSetCurrentMtx stage and additional
FIFO write prove progression beyond GXSetPixelFmt rather than only a hit.

Pinned WiiCompiled at `a135beb201042b20f390c6695ca6b26768820fb4` maps PAL
`0x8016F3E0` in `runtime/src/hle/gx/gx_pixel.cpp` to:

```cpp
GXSetCoPlanar(static_cast<GXBool>(en));
```

The observed hardware tuple is therefore `enable=0`.

## Run health

The latest durable post-main snapshot records:

- PAL main reached and six TaskThread::run hits;
- coherent guest fiber / OS current / OS running at `0x80347498`;
- structurally valid FST at `0x97DC0000`;
- 19 successful DVD reads;
- successful Font.szs decode, producing 3,153,052 bytes;
- 1,556 RMCP01 FIFO writes, one more than the preceding baseline;
- 99 GXCopyDisp calls and 99 successful presents;
- zero present failures.

No native exception report was present among the 28 retrieved text reports.
The logs prove GPU presentation, not visual correctness of the displayed pixels.
MTP supplied no usable timestamps; no root text reports were present before
this nxlink launch, and the reports appeared afterward. Raw diagnostics and
their hashes remain local, outside version control.

## Port decision

Map only PAL GXSetCoPlanar (`0x8016F3E0`) to the pinned one-argument Aurora
call, preserving the direct GXBool cast and guest registers. Aurora performs
the generation-mode update and real FIFO writes. Do not pre-port the following
GXSetClipMode or neighboring GX calls; their status remains hardware-defined.
This document validates GXSetPixelFmt; the new GXSetCoPlanar bridge still
requires its own subsequent Switch run before it is hardware-crossed.
