# Hardware result — Discovery scan / GXSetClipMode frontier (2026-10-02)

The rendered Discovery NRO built from candidate
`9786b78a1a09b91f7db5d13b8de1ce03510d02ab` crosses PAL GXSetCoPlanar
and reaches the next exact unsupported GX call. Its SHA-256 is
`d9eaea1699ac39139d551022cd3301ea3d0470e68f6df03b63d1dda78d21fb4c`.
Nxlink transferred the NRO successfully after the netloader was restarted;
28 text reports were subsequently retrieved through USB/MTP.

## Durable blocker

```text
kind                  : DIRECT
target                : 0x8017351c
lr                    : 0x80240ecc
r3                    : 0x00000000
fast-track stage      : RMCP01_GX_SET_CO_PLANAR
```

The discovery trace records GXSetCoPlanar as unique target 1,240 and the
distinct GXSetClipMode target as unique target 1,241. Both records carry
dispatch count 605,694; the distinct later target, first-hit order and new
stage prove progression beyond GXSetCoPlanar, without requiring the dispatch
counter itself to advance. The translated caller executes GXSetClipMode
after GXSetCoPlanar returns.

Pinned WiiCompiled at `a135beb201042b20f390c6695ca6b26768820fb4` maps PAL
`0x8017351C` in `runtime/src/hle/gx/gx_pixel.cpp` to:

```cpp
GXSetClipMode(static_cast<GXClipMode>(mode));
```

The captured tuple is `mode=0`, which pinned Aurora names `GX_CLIP_ENABLE`.

## Run health and evidence limits

The changed durable post-main snapshot records:

- PAL main reached and six TaskThread::run hits;
- coherent guest fiber / OS current / OS running at `0x80347498`;
- structurally valid FST at `0x97DC0000`;
- 1,556 RMCP01 FIFO writes;
- 99 GXCopyDisp calls and 99 successful presents;
- zero present failures.

The DVD report differs from the baseline and records 19 successful reads.
The post-main snapshot precedes the GXSetCoPlanar call, so its FIFO count
does not measure the bridge's later writes. No native exception report was
present. GPU presentation remains proven; visual pixel correctness is not.

MTP supplies no usable timestamps. The blocker, discovery trace, heartbeat
and post-main snapshot differ from the preceding run, and the new blocker
stage is introduced by the tested candidate. Identical ancillary reports,
including the Font.szs decode report, are retained evidence and cannot be
independently dated from their hashes. Raw reports and retrieval hashes remain
local, outside version control.

## Next boundary

GXSetCoPlanar is hardware-crossed. The next observed boundary is only
GXSetClipMode (`0x8017351C`) with `mode=0`. No neighboring GX calls have
been implemented by this validation step.
