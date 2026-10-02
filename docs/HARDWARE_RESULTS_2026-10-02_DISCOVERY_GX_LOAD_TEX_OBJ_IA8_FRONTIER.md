# Hardware result — Discovery scan / GXLoadTexObj IA8 frontier (2026-10-02)

The rendered Discovery NRO built from code candidate
`990a2412e086d0f2d7cab4441c518eeaad27a979` crosses GXSetChanAmbColor on the
real Switch. Its size is 73,289,784 bytes, SHA-256
`d92132a4004e92ec435990d5f4e514eea99c4cfd78a30f162f273f86718db17a`.
Nxlink completed with exit code 0 at 15:28:20 UTC (17:28:20 Europe/Paris).
USB/MTP retrieval preserved 28 text reports totaling 526,502 bytes.

## Ambient-color progression

The first hit of GXSetChanAmbColor `0x8017039C` is unique target 1,248,
dispatch 606,223: r3=4, r4=`0x80398FD0`, LR=`0x80240F8C`, preceding stage
`RMCP01_GX_SET_NUM_CHANS`. A later distinct translated caller `0x80241240`
appears at dispatch 606,248 with stage `RMCP01_GX_SET_CHAN_CTRL`, followed
by the new durable texture-load blocker. This proves the ambient bridge
returned. The trace records first hits only; the ambient stage itself has
already been superseded by subsequent material-color/channel-control stages.
The color bytes were not captured and are not inferred.

GXSetCoPlanar, GXSetClipMode, GXSetIndTexMtx and GXSetIndTexCoordScale remain
crossed. GXSetDither and GXSetDstAlpha are absent and remain unreached.

## New durable frontier

```text
kind                  : GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR
target                : 0x80170f2c
lr                    : 0x80241260
r3                    : 0x80384500
r4                    : 0x00000000
fast-track stage      : RMCP01_GX_LOAD_TEX_OBJ
```

This is an existing bridge's descriptor gate, rather than a missing direct
mapping. GXLoadTexObj has already crossed on the original RGB565 descriptor;
this later texture is outside its currently accepted exact descriptor.

The readable descriptor has words
`00000190 00000000 00300C03 0001C22A 00000000 00000003 00000000 00010202`.
Pinned extraction resolves data address `0x00384540`, width/height 4/4,
format 3 in both format fields (`GX_TF_IA8`), clamp/clamp, no mipmaps,
linear/linear filters, zero min/max LOD and bias, bias-clamp false,
edge-LOD false and anisotropy 1x. User data and TLUT words are zero;
the non-CI flag is set.

Pinned Aurora's GXGetTexBufferSize computes one 4x4 IA8 block: 32 bytes.
The descriptor's high block count at +0x1C is one. The load status currently
prints `size=0x000B9400` because the refusal path reports the original RGB565
candidate's constant. That field is not the size of this new texture and
does not prove its backing is mapped. The refusal occurs before the backing
range check or native bind. Texture payload bytes were not retrieved.

## Next bounded contract audit

WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`. Its
`gx_objects.cpp` decodes physical texture addresses, filters, LOD and flags
from guest memory. Aurora `GXTexture.cpp` supports IA8 sizing and binding.
Any next candidate must validate the actual 32-byte range, preserve the
decoded state (including edge-LOD false), and bind the real backing through
the existing host texture object. Reusing the RGB565 format, dimensions,
payload size or edge-LOD value would be incorrect. Guest post-load state
must retain the pinned mirror updates. Unknown descriptors must remain
diagnosed until their contract is covered. No new texture-load behavior
is implemented by this result-recording change.

## Health and freshness

The changed post-main snapshot at dispatch 606,238 follows the first ambient
call and precedes the later caller/load frontier. PAL main is reached,
TaskThread::run has six hits, and guest fiber / OS current / OS running all
equal `0x80347498`. The FST at `0x97DC0000` remains structurally valid at
64,224 bytes. The renderer is initialized with an active frame.

The snapshot retains 1,556 FIFO writes, 99 GXCopyDisp calls, 99 successful
presents and zero present failures. GXSetChanMatColor advances from 58 to
60 hits. These counters do not independently measure ambient native XF
writes or prove visual pixel correctness. No native exception report was
retrieved. Changed resource reports retain 19 successful DVD reads and an
SZS decode-pass consuming 499,251 bytes and producing 3,153,052 bytes.

MTP supplies no usable timestamps. Thirteen reports differ from the accepted
baseline: DVD reads, discovery targets, dispatch blocker, texture wrap mode,
texture load, heartbeat history, heartbeat, OS message events, receive-message
frontier, OS sleep events, last post-main dispatch, SZS status and thread events.
The ordered later target, changed post-main stage and new descriptor blocker
establish candidate progression. Identical ancillary reports, including the
LOD report, cannot be independently dated to this run.

Static coverage is 10,949 direct targets: 141 native, 10,494 translated and
314 missing, with 835 runtime-seen direct targets and 19 runtime-seen missing
targets. These counts do not predict the number of future blockers. Raw
reports, retrieval hashes, result JSON and the full bundle remain local.
