# Hardware result — Discovery scan / GXSetIndTexMtx frontier (2026-10-02)

The rendered Discovery NRO built from code candidate
`1406039e9addbeeb7889002faf96cb870934fba3` crosses PAL GXSetClipMode
and stops at GXSetIndTexMtx. The NRO is 73,277,496 bytes, SHA-256
`cfa889d80b8e3a6b4435131142926fcc4801c2dd8b955940b5c73ca408108027`.
Nxlink completed successfully at 13:12:35 UTC; 28 diagnostic text reports
(524,644 bytes) were retrieved through USB/MTP at 13:14:46 UTC.

## Batch acceptance

| Member | First-hit evidence | Result |
| --- | --- | --- |
| GXSetClipMode `0x8017351C` | Unique target 1,241, dispatch 605,878, r3=0 | Hardware-crossed |
| GXSetDither `0x80172930` | Absent from the first-hit trace | Pre-ported; hardware validation pending |
| GXSetDstAlpha `0x8017295C` | Absent from the first-hit trace | Pre-ported; hardware validation pending |

The distinct later target `0x80240F68` is unique target 1,242 at dispatch
605,879 with stage `RMCP01_GX_SET_CLIP_MODE`. Further translated calls and the
new durable blocker prove that GXSetClipMode returned. The previous CoPlanar
boundary remains crossed. The new blocker occurs before this path reaches
GXSetDither or GXSetDstAlpha.

## Durable blocker and pinned attribution

```text
kind                  : DIRECT
target                : 0x80171814
lr                    : 0x80240f88
r3                    : 0x00000001
r4                    : 0x802581f8
r5                    : 0x00000001
fast-track stage      : RMCP01_GX_SET_NUM_IND_STAGES
```

The discovery trace records this as unique target 1,245 at dispatch 605,978.
Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`,
`runtime/src/hle/gx/gx_indirect.cpp`, attributes this target to GXSetIndTexMtx:
r3 is the matrix selector, r4 points to six big-endian float32 values, and r5
is narrowed to signed eight-bit scale exponent. The diagnostic does not
capture the matrix contents; none are inferred here.

Pinned Aurora's `lib/dolphin/gx/GXBump.cpp` maps the selector to a matrix index,
converts the six coefficients to fixed-point fields, emits three BP registers,
and sets `bpSent=1`. Forwarding a guest address directly as a host pointer or
substituting a guessed matrix would not preserve these semantics.

The local translated caller statically initializes three matrix selectors
with this address and exponent, then calls GXSetIndTexCoordScale
(`0x80171968`) for four stages with both scale arguments zero. Only the first
matrix call is observed on hardware. The remaining calls are a continuation
forecast, not runtime acceptance.

## Run health and freshness

The changed durable post-main snapshot at dispatch 605,970 records:

- PAL main reached and six TaskThread::run hits;
- coherent guest fiber / OS current / OS running at `0x80347498`;
- a structurally valid FST at `0x97DC0000`, size 64,224 bytes;
- 1,556 FIFO writes, 99 GXCopyDisp calls and 99 successful presents;
- zero present failures, with the renderer initialized and frame active.

These graphics counters match the previous accepted run. The snapshot follows
GXSetClipMode and precedes the matrix blocker; it is not a measurement of
GXSetClipMode's own FIFO writes. No native exception report was retrieved.
GPU presentation remains proven; visual pixel correctness remains unverified.

MTP supplies no usable timestamps. Six reports differ from the baseline:
discovery targets, dispatch blocker, heartbeat history, heartbeat, OS sleep
events, and the last post-main dispatch. The new ClipMode stage and later
distinct targets establish attributable progression. Identical ancillary
reports, including DVD and graphics initialization reports, cannot be dated
independently and do not establish new DVD/resource activity.

The local coverage scan reports 10,949 direct targets: 138 native, 10,494
translated and 317 missing. It correlates 831 direct targets with this trace,
including 20 missing targets. These counts are not guaranteed future blockers.
Raw reports, hashes and the complete bundle remain local and uncommitted.

## Next bounded candidate

The observed matrix boundary requires checked guest-memory access, exact
big-endian float decoding, the pinned selector/exponent conversions, a distinct
stage, executable decoding/context-preservation tests, and the rendered gates.
Invalid memory must retain a durable hard stop. Aurora's float-to-integer
conversion also needs an explicit audit for invalid coefficients.

GXSetIndTexCoordScale is the adjacent scalar candidate: its pinned wrapper
forwards all three enum casts; Aurora updates one of two cached scale words,
emits the selected BP register and sets `bpSent=1`. Its stage selection and
field conversion must be tested independently before including it in the next
candidate. Other guest-memory boundaries remain hardware-driven.
