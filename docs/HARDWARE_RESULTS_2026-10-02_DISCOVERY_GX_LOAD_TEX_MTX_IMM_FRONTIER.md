# Hardware result — IA8 maps crossed / GXLoadTexMtxImm frontier (2026-10-02)

Code candidate `f5d9fdd` produced the 73,289,784-byte rendered Discovery NRO,
SHA-256 `3cfba800770622a32e843e4869c50d00c1213e861cd24c8c1f6424ab607d0e16`.
Nxlink completed with exit code 0 at 16:41:04 UTC (18:41:04 Europe/Paris).
USB/MTP retrieval preserved 28 text reports totaling 526,666 bytes.

## Eight IA8 loads returned

The changed load status records `load-pass`, object `0x80384500`, map ID 7,
backing `0x00384540`, 4x4 IA8 and the correct 32-byte size. This is supported
by a later distinct caller, not only by the status file:

| Evidence | Dispatch | Stage |
| --- | --- | --- |
| IA8 caller `0x80241240` | 607,459 | `RMCP01_GX_SET_CHAN_CTRL` |
| Later caller `0x802412C8` | 607,516 | `RMCP01_GX_LOAD_TEX_OBJ` |
| New target `0x80173234` | 607,523 | `RMCP01_GX_SET_NUM_TEX_GENS` |

The audited local `0x80241240` caller binds the same descriptor to maps 0..7
in order before returning to its parent. The parent then calls `0x802412C8`.
Together with the map-7 status and new frontier, this establishes that all
eight exact IA8 loads returned on the Switch. It does not validate arbitrary
descriptors, other RGB565 map IDs, payload pixel values or visual rendering.

## New direct frontier and pinned audit

```text
kind                  : DIRECT
target                : 0x80173234
lr                    : 0x80240f94
r3                    : 0x802581c8
r4                    : 0x0000001e
r5                    : 0x00000000
fast-track stage      : RMCP01_GX_SET_NUM_TEX_GENS
```

WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`.
Its gx_transform.cpp maps this to GXLoadTexMtxImm: r3 is a guest matrix
address, r4 is the matrix ID (captured 30), r5 is type (captured 0, 3x4).
The observed type requires 12 sequential big-endian float32 coefficients,
48 bytes, decoded into a zero-initialized host array. No coefficients were
captured or inferred. The pinned wrapper does not activate the frame.

Aurora GXTransform.cpp validates regular IDs 30..60 or post-texture IDs
64..125. Post-texture IDs require type 3x4 and select XF address
`(id-64)*4+0x500`; regular IDs select `id*4`. It emits command 0x10,
an XF register word and 8 floats for type 2x4, otherwise 12. The pinned
wrapper reads 12 only for type 3x4 and 8 otherwise, leaving its other four
array entries zero; types outside 0/1 therefore need explicit audit rather
than silent normalization. The captured type 0 has matching 12-value counts.
Neither wrapper nor Aurora supplies EnsureAuroraFrameActive here.

The immediate caller statically prepares ten type-0 matrix loads using the
same address and IDs 30,33,...57. These repeats remain a forecast, not a
record of ten hardware hits. Any next bridge needs checked matrix access,
exact endian/order/ID/type forwarding, CPU preservation and its own
executable/rendered/hardware validation. No matrix behavior is added by
this result-recording change.

The close-path audit also identifies coordinate setup, TEV and pixel-state
initialization after these matrices. Some direct entries remain absent and
GXSetTexCoordGen2 currently accepts only destination coordinate 0, while the
local loop prepares 0..7. These are static scope forecasts, not permission
to guess state or proof that every forecast will become a blocker. They
explain why a successful texture bind alone cannot predict the first game
image or a completion date.

## Health and attribution

The changed post-main snapshot at dispatch 607,449 precedes the eight IA8
loads. It records main reached, six TaskThread::run hits, coherent guest
fiber / OS current / running at `0x80347498`, a structurally valid FST at
`0x97DC0000` of 64,224 bytes, initialized renderer and active frame.
It retains 1,556 FIFO writes, 99 GXCopyDisp calls, 99 successful presents
and zero present failures. Because this snapshot precedes the binds, its
FIFO/present counters cannot prove their native effects or visible pixels.
No native exception report was retrieved.

Seven reports differ from the baseline: discovery targets, dispatch blocker,
texture load, heartbeat history, heartbeat, OS sleep events and the last
post-main dispatch. MTP supplies no usable timestamps; the changed map-7
status and distinct later targets establish progression. DVD and SZS reports
are identical to the baseline and are not independently fresh evidence.
Static coverage remains 141 native, 10,494 translated and 314 missing among
10,949 direct targets, with 837 runtime-seen and 20 runtime-seen missing.
These counts do not predict remaining runtime blockers. Raw reports, hashes,
analysis JSON and bundle remain local. Visual game pixels remain unverified.
