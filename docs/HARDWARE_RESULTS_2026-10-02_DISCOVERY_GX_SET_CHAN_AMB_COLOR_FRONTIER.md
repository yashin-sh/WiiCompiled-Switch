# Hardware result — Discovery scan / GXSetChanAmbColor frontier (2026-10-02)

The rendered Discovery NRO built from code candidate
`c6b7a209f593e987b0e147164cb39a00a4b16ed6` crosses GXSetIndTexMtx and
GXSetIndTexCoordScale on the real Switch. Its size is 73,281,592 bytes,
SHA-256 `9baf0d8141be506e2c75bbc63217466c6412f777cf6b3aea95419d9deddedc4b`.
Nxlink completed successfully after the netloader was restarted. USB/MTP
retrieval preserved 28 text reports, totaling 525,608 bytes.

## Per-boundary progression

| Boundary | First hit | Later distinct evidence | Result |
| --- | --- | --- | --- |
| GXSetIndTexMtx `0x80171814` | Unique target 1,245, dispatch 605,883; r3=1, r4=`0x802581F8`, r5=1 | GXSetIndTexCoordScale, dispatch 605,892, stage `RMCP01_GX_SET_IND_TEX_MTX` | Hardware-crossed |
| GXSetIndTexCoordScale `0x80171968` | Unique target 1,246, dispatch 605,892; r3/r4/r5=0 | Translated caller `0x802410EC`, dispatch 605,902, stage `RMCP01_GX_SET_IND_TEX_COORD_SCALE`, then the new durable blocker | Hardware-crossed |

The ordered later targets prove that both bridges returned. The matrix
contents were not captured; this result does not establish a particular
matrix value. The caller's statically audited three-matrix/four-scale loops
are consistent with the subsequent translated caller, but the first-hit
trace does not independently log each repeated invocation.

GXSetClipMode remains crossed. GXSetDither and GXSetDstAlpha are absent from
this trace and remain pre-ported, with hardware validation pending.

## Durable blocker and next pinned audit

```text
kind                  : DIRECT
target                : 0x8017039c
lr                    : 0x80240f8c
r3                    : 0x00000004
r4                    : 0x80398fd0
fast-track stage      : RMCP01_GX_SET_NUM_CHANS
```

The blocker is unique target 1,248 at dispatch 605,915. Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps it in
`runtime/src/hle/gx/gx_lighting.cpp` to GXSetChanAmbColor. The wrapper first
calls EnsureAuroraFrameActive, reads a guest big-endian color word from r4,
decodes the four RGBA bytes, and forwards r3 as GXChannelID. The captured
channel is 4 (`GX_COLOR0A0`); the color word is not captured and is not guessed.

Pinned Aurora's `lib/dolphin/gx/GXLighting.cpp` expands the combined channel
into its color/alpha channel calls, packs the RGBA value, updates cached
ambient color, emits the XF ambient-color register and clears `bpSent`.
Its channel check must remain effective. Frame activation must also be
preserved even though this run's preceding snapshot reports an active frame.

The existing GXSetChanMatColor bridge provides an audited counterpart for
the frame helper and RGBA decoding. The next candidate still requires its
own stage, checked color access, executable argument/context-preservation
tests, rendered gates and exact-NRO hardware proof. No ambient-color bridge
is implemented by this result-recording change.

## Health, resource evidence and freshness

The changed durable post-main snapshot at dispatch 605,915 records PAL main
reached, six TaskThread::run hits, coherent guest fiber / OS current / OS
running at `0x80347498`, and a structurally valid FST at `0x97DC0000` of
64,224 bytes. The renderer is initialized and the frame active.

It retains 1,556 FIFO writes, 99 GXCopyDisp calls, 99 successful presents and
zero present failures. GXSetNumIndStages and GXSetNumChans each have 142 hits
(previously 141). The snapshot follows both new bridges and precedes the
ambient-color call. The FIFO counter does not independently measure Aurora's
native matrix/scale register effects. Presentation does not establish visual
pixel correctness. No native exception report was retrieved.

The changed DVD report records 19 successful reads. The changed SZS report
records decode-pass with 499,251 source bytes consumed and 3,153,052 output
bytes produced. Neither resource completion means the game's UI is visually
correct or fully runnable.

MTP supplies no usable timestamps. Twelve reports differ from the accepted
baseline: DVD reads, discovery targets, dispatch blocker, texture wrap mode,
heartbeat history, heartbeat, OS message events, receive-message frontier,
OS sleep events, last post-main dispatch, SZS decode status and thread events.
The two new stages and ordered later targets establish attributable candidate
progression. Identical ancillary reports cannot be independently dated.

The static scan reports 10,949 direct targets: 140 native, 10,494 translated,
315 missing, with 834 runtime-seen direct targets and 20 runtime-seen missing
targets. These counts are not a count of guaranteed future blockers. Raw
reports, retrieval hashes and the full bundle remain local and uncommitted.
