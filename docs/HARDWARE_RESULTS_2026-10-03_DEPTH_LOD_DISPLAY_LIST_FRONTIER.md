# Hardware result — depth LOD returned, display-list frontier (2026-10-03)

The [depth-LOD fix](GX_DEPTH_LOD_2026-10-03.md), code
`b5f0a2b0d50266e7538f76cd3438306c3a6293e9`, produced the 73,396,280-byte
Rendered Discovery NRO, SHA-256
`596ba38a52d588b61eb1edff241d3b6969c8a2c459e70d849a3236f3ed02551a`.
All five exact-code workflows / six jobs, actual depth/native contract logs,
private build, 35 strong symbols and scoped Init/LOD provider checks passed.
Subsequent documentation revision `eb61bc53f005d496cfdeeaaccf7767ae2416482c`
changes only Markdown. Candidate source/artifact hashes, dependency pins and
the user's integration patch bytes/nine mtimes were reverified before testing.

The earlier 20:05:21 UTC transfer attempt failed to connect and is not a
console result. After the user confirmed netloader readiness again, fresh
UDP discovery received `bootnx` from `192.168.1.194:28280` at **20:48:49 UTC**.
No TCP preflight consumed the connection. Nxlink transferred the exact NRO
with **exit 0 at 2026-10-03 20:49:11.080745 UTC** (22:49:11 Europe/Paris).
Its displayed 36.38% is the compressed-to-original byte ratio.

USB/MTP retrieval at **20:52:40 UTC** preserved **28 reports / 535,022 bytes**.
Twelve changed versus the preceding Fog/ZCompLoc run; sixteen are identical
and cannot independently be dated. Source timestamps are unavailable. All
manifest sizes, SHA-256 hashes and baseline deltas, plus the full ZIP CRC,
were independently verified. The changed successful LOD report, new discovery
continuation, distinct blocker and coherent heartbeat cohort attribute this
run to the transferred candidate. Raw diagnostics/products/checked analysis
remain local under `.deps/network-tests/gx-depth-lod/runs/20261003T205240Z/`.

**The user confirms a black screen followed by an error for this run.** Exact
on-screen wording/code was not supplied. Independently, the durable report
records an intentional unsupported-DIRECT abort at GXBeginDisplayList; it does
not establish the displayed error text. Recognizable game pixels remain
unproven.

## Accepted depth LOD return

The fresh report records `status=lod-pass`, object **`0x80384170`**, min/mag
filters **0/0**, min/max/bias f32 bits **`00000000`**, and biasClamp/edgeLod/
aniso **0/0/0**. Its guest mode words are **`00000105 / 00000000`**,
matching the audited zero/nearest fixture `0x95 → 0x105`.

The implemented status is written after native forwarding returns and the
guest mode-word readback checks succeed. The checked constructor performs
init, LOD and inline matrix setup before invoking the next constructor.
Fresh coherent later entries establish its continuation:

| First-hit entry | Dispatch | Context |
| --- | --- | --- |
| Texture constructor `0x8021A4D8` | 608988 | LR `0x802291BC`, r1 `0x80399008`, DstAlpha stage |
| Following constructor `0x8021B6A4` | 609003 | LR `0x8021A568`, r1 `0x80398FF8`, LOD stage |
| GXBeginDisplayList `0x80172E00` | 609010 | LR `0x8021A568`, r1 `0x80394E40`, LOD stage |

The following constructor and blocker share r2 `0x8038EFA0`, r13 `0x8038CC00`
and fiber `0x80347498`. Its checked prologue aligns/allocates its stack frame,
then prepares a buffer at r1+`0xC0` for the display-list call, agreeing with
the actual later registers. Thus **GXInitTexObjLOD returned on the observed
4×4 Z24X8 object and zero/nearest arguments**. Discovery supplies first hits,
not a dedicated return trace. Native graphics effects and guest words2..7
were not separately captured after LOD on this run; their unchanged-write
contracts remain host evidence.

The init report is byte-identical to the preceding run and cannot be newly
dated in isolation. Current init progression follows from the fresh checked
constructor path, successful cached-host LOD operation and later continuation.
The earlier accepted IA8/matrix/coordinate/TEV/AlphaCompare/Fog/ZCompLoc/pixel
setup path remains crossed. Alternate dimensions, arguments, strict mode,
refusal paths and native bit-preservation fixtures retain host acceptance.

## New display-list boundary

GXBeginDisplayList **arrived and did not return**. The durable report records:

- kind **DIRECT**, target **`0x80172E00`**;
- dispatch **609010**, elapsed **108440 ms** from the first translated dispatch;
- LR **`0x8021A568`**, r1 **`0x80394E40`**;
- r3 / buffer **`0x80394F00`**, r4 / capacity **`0x00004000` = 16 KiB**;
- stage **`RMCP01_GX_INIT_TEX_OBJ_LOD`**;
- action **abort after durable blocker record**.

The captured buffer is 32-byte aligned. Buffer readability/content and native
recording were not measured by this DIRECT diagnostic. Zero-valued inactive
LOD fields in the blocker do not describe the earlier successful texture:
the target is now display-list begin, so those diagnostics are not selected.

The [candidate's static audit](GX_DEPTH_LOD_2026-10-03.md) predicted this call;
it is now a hardware arrival. Pinned Aurora begins bounded FIFO recording,
flushes dirty state and optionally saves GX shadow state. Its later
GXEndDisplayList returns a rounded byte length and restores saved state.
These stateful effects need a coordinated port/audit; no no-op or unrelated
behavior was added in this result commit. GXEndDisplayList and the later
display-list/drawing path remain static forecasts, not console acceptance.

## Health and remaining scope

The watchdog records **103 samples: 102 ACTIVE and one STALE**, maximum
sampling interval **2168 ms**. The isolated STALE sample at 12.790 seconds is
followed by ACTIVE progression. The 108.440-second startup elapsed value is
not representative-scene performance evidence or an ETA to recognizable
pixels. No native exception report was retrieved; absence is not exhaustive
proof.

The changed preceding heartbeat at dispatch **608302**, VtxAttrFmt stage,
records 1556 guest FIFO writes and 99 CopyDisp/successful presents / zero
failures. The later post-main snapshot at **608722**, after pixel setup but
before the new texture constructor, records **1558** guest FIFO writes,
last word **`E8000156`**, and the same 99 presents. Both retain 3826 StaticR
dispatches, six TaskThread runs, valid 64,224-byte FST and initialized renderer
with active frame. The graphics report itself is byte-identical. These
preceding counters do not establish a visible frame after LOD or count each
native state command separately.

Recognizable game content, depth upload/decoding, display-list recording and
replay, sustained error-free execution, input/audio correctness,
representative performance and whole-link duplicate ownership remain open.
Earlier dated records retain their original acceptance scope.
