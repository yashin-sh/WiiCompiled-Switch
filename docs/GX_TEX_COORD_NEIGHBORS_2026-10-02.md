# Texture-coordinate neighbors: static audit, 2026-10-02

This is a source audit for a possible later fix. It adds no implementation,
records no new hardware frontier, and does not establish that the calls below
have returned on the Switch. The current hardware frontier remains the
previously recorded `GXLoadTexMtxImm (0x80173234)` until a new run supplies
durable evidence.

The audited WiiCompiled pin is
`a135beb201042b20f390c6695ca6b26768820fb4`, including its Aurora source tree.
The existing nine-file Switch integration patch does not modify the audited
wrapper or Aurora implementation bodies.

## Source references

| Contract | Source and line at the audited pin |
| --- | --- |
| Scale and bias wrappers | `third_party/WiiCompiled/runtime/src/hle/gx/gx_texture.cpp:704` and `:707` |
| Guest GXData pointer slot | `third_party/WiiCompiled/runtime/src/hle/gx/gx_internal.h:58` |
| Gen2 wrapper | `third_party/WiiCompiled/runtime/src/hle/gx/gx_vertex.cpp:201` |
| Aurora scale and bias | `third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTexture.cpp:449` and `:470` |
| Aurora Gen2 | `third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXGeometry.cpp:295` |
| Aurora matrix-index emission | `third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXManage.cpp:492` |
| FIFO/register macros | `third_party/WiiCompiled/aurora-main/lib/dolphin/gx/__gx.h:15`, `:35`, `:68` |
| GXBool and coordinate enum | `third_party/WiiCompiled/aurora-main/include/dolphin/gx/GXEnum.h:10` and `:219` |
| Existing bounded Gen2 bridge | `source/gx_set_tex_coord_gen2_hle_bridge.cpp:17` and `:35` |
| Local static caller | `local-product/generated/functions/func_802412C8.cpp:45` and `:72` |

## Static caller forecast

The local function `0x802412C8` first sets one texture generator, then calls
`GXLoadTexMtxImm` ten times with the same matrix address `0x802581C8`, type
`0`, and IDs `30,33,...,57`. It then contains a loop over `coord=0..7`:

| Order in each loop iteration | Target | Arguments |
| --- | --- | --- |
| 1 | `GXSetTexCoordGen2`, `0x8016E37C` | `(coord,1,4,60,0,125)` |
| 2 | `GXSetTexCoordScaleManually`, `0x80171180` | `(coord,0,0,0)` |
| 3 | `GXSetTexCoordBias`, `0x801711FC` | `(coord,0,0)` |

These constants and the loop are a static forecast. They are not eight
hardware hits, nor proof that any scale/bias call has been reached. If this
path executes as written, scale and bias for coord 0 occur before the Gen2
variation for coord 1.

The current Gen2 bridge accepts only
`(0,1,4,60,0,125)`:
`GX_TEXCOORD0/GX_TG_MTX2x4/GX_TG_TEX0/GX_IDENTITY/GX_FALSE/GX_PTIDENTITY`.
It sets stage `RMCP01_GX_SET_TEX_COORD_GEN2`; a variation records
`GX_SET_TEX_COORD_GEN2_UNPROVEN_ARGS` against target `0x8016E37C` and aborts.
A null CPU returns without effects. Its headless form retains the guard and
does not call Aurora. Therefore the existing native trait does not cover
coords 1..7 merely because it is available.

## Arguments and conversions

| Wrapper | PPC registers | Native conversions |
| --- | --- | --- |
| Scale manually | `r3=c`, `r4=en`, `r5=ss`, `r6=ts` | `(GXTexCoordID)c`, `(GXBool)en`, `(u16)ss`, `(u16)ts` |
| Bias | `r3=c`, `r4=se`, `r5=te` | `(GXTexCoordID)c`, `(GXBool)se`, `(GXBool)te` |
| Gen2 | `r3=dst`, `r4=type`, `r5=src`, `r6=mtx`, `r7=normalize`, `r8=postMtx` | coordinate/type/source enums; `mtx` and `postMtx` remain `u32`; normalize becomes `GXBool` |

The rendered Switch build and host contracts define `TARGET_PC`. At this
pin that makes `GXBool` a **bool**, not an enum or an eight-bit integer.
The native casts therefore map every nonzero argument to true. The guest
mirrors below use the original 32-bit arguments and their low bit. For
example, raw `en=2` enables the native scale path but clears the guest manual
bit; raw `se=2` sets the native bias bit but clears its guest counterpart.
Do not silently replace raw mirror operands with the converted bool.

Without `TARGET_PC`, the header instead defines `GXBool` as `u8`; that is a
different conversion contract and is not the rendered configuration audited
here. Also, accepting an arbitrary integer for a C++ enum is not made safe
by forwarding it through a stub: coordinate bounds must precede casts and
native indexing, and any supported type/source expansion needs its own
scope review.

## Guest mirror: exact order and masks

Both wrappers call Aurora **before** entering their guest-memory `try` block.
They read `gd = Memory::Read32(0x803886C8)`. If `gd==0`, the native call has
already happened but no guest mirror is written. Every exception in the
mirror is caught and discarded. Writes already completed before an exception
remain completed; the pin does not roll them back or prevalidate an entire
structure.

For `c`, define `S=gd+0x108+c*4`, `T=gd+0x128+c*4`, and `E=gd+0x5E4`.
The pin calculates these addresses with unsigned 32-bit expressions.

Scale manually executes the following mirror operations in order:

1. `Write32(E, (Read32(E) & ~(1u<<c)) | ((en&1u)<<c))`.
2. Only when raw `en!=0`, write S as
   `(Read32(S)&0xFFFF0000u) | ((ss-1u)&0xFFFFu)`.
3. Under the same condition, write T as
   `(Read32(T)&0xFFFF0000u) | ((ts-1u)&0xFFFFu)`.
4. Under the same condition, `Write16(gd+2,0)`.

The size subtraction wraps in unsigned arithmetic. A zero size encodes
`0xFFFF`; the native path first narrows the size to `u16`, producing the
same low-16-bit result. Disabled scale changes only the manual bit: it does
not overwrite S/T sizes or the halfword at `gd+2`.

Bias executes these mirror operations in order:

1. Write S as `(Read32(S)&0xFFFEFFFFu) | ((se&1u)<<16)`.
2. Write T as `(Read32(T)&0xFFFEFFFFu) | ((te&1u)<<16)`.
3. If `Read32(E)&(1u<<c)` is nonzero, `Write16(gd+2,0)`.

This preserves every S/T bit except bit 16. Neither wrapper changes the
guest dirty-state word at `gd+0x5FC`, nor adds any other dirty flag. The Gen2
wrapper has **no guest GXData mirror**: after optional invalid-source logging,
it calls Aurora directly. In particular, it does not write guest matrix-index
shadows or a guest dirty-state word.

## Aurora shadow state, FIFO and frames

Scale updates the native eight-bit `tcsManEnab` mask. When its converted
enable is true, it replaces the low 16 bits of native `suTs0[c]` and
`suTs1[c]` with the narrowed sizes minus one, sends those two BP registers,
and sets native `bpSent=1`. Disabled scale sends no FIFO commands.

Bias changes bit 16 of native S/T. It sends the same two BP registers and
sets `bpSent=1` only when native `tcsManEnab` contains the coordinate bit.
Each BP command is opcode `0x61` followed by one 32-bit register word;
the pair is 10 FIFO bytes. Initialized S/T register IDs are respectively
`0x30+c*2` and `0x31+c*2`. The native `bpSent=1` and guest halfword `+2=0`
are intentionally different in the pinned code; do not make them identical
without a separate justified change.

Gen2 checks `dst` against coordinates 0..7. It emits XF registers
`0x1040+dst` and `0x1050+dst`, updates a six-bit matrix-index field in
`matIdxA` for coords 0..3 or `matIdxB` for coords 4..7, then emits that shadow
through CP register `0x30` or `0x40` and XF register `0x1018` or `0x1019`.
`__GXSetMatrixIndex` sets native `bpSent=0`. For the forecast tuple the
first XF value is `0x280`, the post-transform value is `0x3D`, and the
matrix-index value being inserted is 60. The register macro clears the
destination field but does **not** mask an arbitrary incoming value to
the field width; these exact tuple values fit their fields.

None of these wrapper/native paths calls `EnsureAuroraFrameActive`, begins
or presents a frame, or calls `GXMarkFrameWork`. Their FIFO/state work must
not be treated as a recognizable game image or proof of visible pixels.
Scale and Bias have no native coordinate CHECK before shifts/array access;
the caller contract must prevent coordinates outside 0..7.

## Future fix and validation scope

No next bridge is implemented by this audit. A future observed frontier can
be compared against the forecast, then given a bounded contract. Keep these
points explicit:

- Validate the supported coordinate range before shifts, enum conversion and
  native array indexing. A native enum sentinel such as `GX_TEXCOORD_NULL`
  is not a safe array index.
- Preserve the native-before-mirror ordering, raw versus converted boolean
  semantics, masks and conditional writes. Do not invent a dirty-state
  update or a frame-start helper.
- Check guest ranges and address additions in wide arithmetic when adding
  validation. Document any deliberate change from the pin's swallowed
  failures/partial-write behavior rather than claiming it is identical.
- Use the real Switch Memory implementation with synthetic mapped storage
  to test big-endian mirror writes, all supported coords, neighboring-bit
  preservation, size narrowing/underflow, zero and nonzero boolean arguments,
  manual-enabled/disabled bias, null CPU and CPU preservation.
- Test absent/null/short/unmapped GXData backing and near-`2^32` address
  additions according to the explicitly chosen failure contract. A stub
  receiving forwarded arguments does not prove Aurora FIFO contents or
  out-of-enum behavior.
- Require a later distinct dispatch or attributable durable frontier from
  the exact candidate NRO before marking any new call hardware-crossed.

