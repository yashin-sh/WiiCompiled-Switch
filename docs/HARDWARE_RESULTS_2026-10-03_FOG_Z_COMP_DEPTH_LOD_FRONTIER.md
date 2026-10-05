# Hardware result — Fog and ZCompLoc returned, depth-texture LOD frontier (2026-10-03)

The [Fog/ZCompLoc candidate](GX_FOG_Z_COMP_2026-10-03.md), runtime revision
`c329b6d0c163a415a6db0ef819b052d1d9c6c88a`, produced the 73,396,280-byte
Rendered Discovery NRO, SHA-256
`652afed471c2fc8ba9aa6735612fbe1ac69295aecf7b9a0dbfd6505dfb1024fb`.
Optimized host-test revision `e24480511513899e7ae242343f16cccfebfa244e`
retains the exact runtime and NRO bytes. Both revisions passed all five
workflows / six jobs. The private build, 31 strong text symbols and scoped
native-provider checks are documented in the candidate record.

The launch helper reverified candidate sources, dependency pins, the existing
integration patch and NRO digest. UDP discovery identified `192.168.1.194`;
no separate TCP preflight consumed the netloader connection. Nxlink completed
with **exit 0 at 2026-10-03 19:20:07.201856 UTC** (21:20:07 Europe/Paris).
The displayed 36.38% is the compressed-to-original byte ratio, not an
incomplete transfer.

USB/MTP retrieval at **19:23:51 UTC** preserved 28 reports / 534,351 bytes.
Fourteen differ from the preceding AlphaCompare baseline; fourteen are
byte-identical and cannot be independently dated. Source timestamps are
unavailable. All manifest sizes, hashes and baseline deltas, plus the full
ZIP CRC, were independently verified. The coherent new discovery tail,
blocker, texture-init/LOD and heartbeat records attribute this progression
to the transferred candidate. Raw diagnostics, products and checked analysis
remain private under `.deps/network-tests/gx-fog-z-comp/runs/20261003T192351Z/`.

The current run's on-screen observation and exact error wording have not yet
been supplied. Earlier runs were user-confirmed black with an error; that
observation is not automatically assigned to this run. Recognizable game
pixels remain unproven.

## Accepted continuation

| First-hit entry | Dispatch | Captured arguments / preceding stage |
| --- | --- | --- |
| AlphaCompare `0x80172088` | 609039 | `(7,0,0,7,0)`, BlendMode |
| Fog `0x801722CC` | 609047 | type 0, color pointer `0x80398FD0`, ZMode |
| Existing FogRangeAdj `0x80172658` | 609054 | `(0,0,0)`, Fog |
| ZCompLoc `0x80172858` | 609061 | r3=1, r6=`E8000156`, Fog |
| Existing DstAlpha `0x8017295C` | 609062 | `(0,0)`, ZCompLoc |
| Outer pixel setup `0x802415E8` | 609063 | restored r1=`0x80398FD8`, DstAlpha |
| Later caller `0x800771C0` | 609074 | LR=`0x80240EEC`, r1=`0x80398FE8`, DstAlpha |
| Texture constructor `0x8021A4D8` | 609376 | LR=`0x802291BC`, r1=`0x80399008`, DstAlpha |

Fog, FogRangeAdj, ZCompLoc and DstAlpha share LR `0x80240F9C`,
r1 `0x80398FC8`, r2 `0x8038EFA0`, r13 `0x8038CC00` and fiber `0x80347498`.
The checked local caller invokes these in that order and restores its stack
before returning to the outer setup. Its subsequent existing ColorUpdate,
AlphaUpdate, Dither and DstAlpha calls precede the coherent later callers.
The fresh distinct entries and durable post-main snapshot therefore establish
**the admitted Fog call and ZCompLoc(1) returned**, along with the intervening
disabled FogRangeAdj and existing pixel setup. Discovery records first hits;
this is control-flow acceptance, not a separate trace of each return.

Fog's exact f64 guard passed. Its parameter bits and RGBA were captured in
[the preceding run](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md),
not newly dumped here. There is no independent console decode of native Fog
or ZCompLoc BP writes. The existing disabled FogRangeAdj emits guest BP
`E8000156`; its checked caller effects agree with later register context and
FIFO diagnostics. Alternate bridge inputs retain host evidence only.
Earlier accepted IA8, matrix, coordinate, scalar/color and AlphaCompare
progression remains crossed on this path.

## New depth-texture boundary

`GXInitTexObjLOD` **arrived and did not return**. The durable report records:

- kind `GX_INIT_TEX_OBJ_LOD_INVALID_DESCRIPTOR`, target `0x80170A4C`;
- dispatch **609384**, elapsed **109572 ms** from the first translated dispatch;
- LR `0x8021A52C`, r1 `0x80398FF8`, object `0x80384170`;
- complete 32-byte descriptor readable; stage `RMCP01_GX_INIT_TEX_OBJ_LOD`;
- min/mag filters **0/0**, min/max/bias f32 bits all `00000000`,
  biasClamp/edgeLod/aniso **0/0/0**;
- action **abort after durable blocker record**.

The fresh preceding init report says `init-pass`: object `0x80384170`,
data `0x802A2B60`, **4×4**, format **22 / `GX_TF_Z24X8`**, wrap **1/1**,
mipmap **0**. This status is written after native initialization returns.
The separately changed wrap report concerns another object; it does not prove
an intervening wrap call on this depth texture. Zero-valued load-texture
fields in the blocker are unrelated inactive diagnostics, not its data address.

```text
word 0/1: 00000095 / 00000000
word 2/3: 00600C03 / 0001515B
word 4/5: 00000000 / 00000016
word 6/7: 00000000 / 00010302
```

Read-only source inspection identifies the failing guard:
`GetTexObjBlockLayout` in `source/gx_init_tex_obj_hle_bridge.cpp` lacks full
format 22. The pinned GX enum defines `GX_TF_Z24X8` as `6 | 0x10`, so this
label does **not** establish a corrupt descriptor. Its remaining fields agree
with 4×4 dimensions, low format nibble 6, one block, block type 3 and flags 2.
The generic init path uses the low format nibble for guest metadata, whereas
the LOD validator checks the full format.

The candidate's actual rendered compile definitions leave
`MKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES` undefined, hence false. In the optional
strict configuration, the new descriptor and nearest-filter arguments would
also require admission separately; removing the structural failure alone
would not satisfy that configuration. The next change needs pinned layout,
native LOD and guest-word contracts with invalid-descriptor refusal retained.
Depth texture loading/decoding and the later drawing path are not accepted by
this initialization observation. No correction to that guard is included in
this hardware-result commit.

## Health and limits

The watchdog contains **102 samples: 98 ACTIVE and four STALE**, with maximum
sampling interval **2708 ms**. Each isolated STALE sample is followed by
ACTIVE progression. This is not sustained-scene performance evidence, and
109.572 seconds does not predict time to the first game image. No native
exception report was retrieved; absence is not exhaustive proof.

The preceding heartbeat at dispatch **608712** records 1556 guest FIFO writes,
99 CopyDisp calls, 99 successful presents and zero failures. The changed later
post-main snapshot at dispatch **609104**, after the pixel setup, records
**1558** guest FIFO writes, last word **`E8000156`**, and the same 99 presents.
Both retain coherent scheduler state, 3826 StaticR dispatches, six TaskThread
runs, a valid 64,224-byte FST and an initialized renderer with active frame.
The graphics report itself is byte-identical to the baseline. These counters
do not establish a new visible frame or separately count native BP commands.

Visible game content, sustained execution without an error, input/audio
correctness, representative performance, alternate console inputs and
whole-link duplicate ownership remain open. Earlier dated records retain
their original acceptance scope.
