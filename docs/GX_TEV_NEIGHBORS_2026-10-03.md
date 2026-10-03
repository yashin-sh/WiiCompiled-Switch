# TEV neighbors: static contract audit (2026-10-03)

This audit prepares nine entries called by `func_80241380`. It adds no HLE
implementation. The [latest retrieved coordinate run](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md)
now reaches `0x80241380` at dispatch 605620, after all eight Gen2/Scale/Bias
triples with Scale/Bias disabled return. It stops at **GXSetTevDirect (`0x80171B58`),
stage 0**, dispatch 605633, after 100205 host milliseconds, LR `0x80240F98`.
That arrival is observed; Direct has not returned. The other eight audited
entries and the rest of the stage loop remain static forecasts. The earlier
matrix-to-Scale reports remain historical evidence, not the current frontier.

The runtime and bundled Aurora pin is
`a135beb201042b20f390c6695ca6b26768820fb4`. Sources inspected:

- `third_party/WiiCompiled/runtime/src/hle/gx/gx_tev.cpp`, lines 5-128.
- `third_party/WiiCompiled/runtime/src/hle/gx/gx_indirect.cpp`, lines 31-32.
- `third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTev.cpp`, lines 38-109,
  202-219 and 244-265; `GXBump.cpp`, lines 110-132.
- Aurora `GXEnum.h`, `GXTev.h`, `__gx.h`, `GXManage.cpp`, `internal.hpp` and
  `gx/fifo.hpp` at that same pin.
- Local generated `local-product/generated/functions/func_80241380.cpp`,
  lines 27-240, for caller order and argument forecasts only.
- `.deps/network-tests/gx-load-tex-mtx-imm/runs/20261002T174349Z/` for the
  historical matrix-to-Scale frontier and coverage, and
  `.deps/network-tests/gx-tex-coord-batch/runs/20261003T090340Z/` for the current
  coordinate-to-Direct arrival; those reports remain local.

## Entry contracts

All nine pinned wrappers are void. Their arguments are `uint32_t`, taken in
order from r3 onward. Enum conversions retain those argument values; none of
these wrappers first narrows an ID, input selector or operation to a byte.
Only `clamp` is converted to `GXBool`: with `TARGET_PC`, this is C++ `bool`,
so any nonzero 32-bit value becomes true. It is not an `u8` truncation.
The wrappers do not write a guest return register or guest memory.

| Address | Native function | Guest arguments | Pinned wrapper guard |
| --- | --- | --- | --- |
| `0x80171B58` | GXSetTevDirect | r3 stage | None in `gx_indirect.cpp` |
| `0x80171CE0` | GXSetTevColorIn | r3 stage; r4-r7 color inputs a,b,c,d | stage < 16 |
| `0x80171D60` | GXSetTevColorOp | r3 stage; r4 op; r5 bias; r6 scale; r7 clamp; r8 output register | stage < 16 and output register < 4 |
| `0x80171D20` | GXSetTevAlphaIn | r3 stage; r4-r7 alpha inputs a,b,c,d | stage < 16 |
| `0x80171DB8` | GXSetTevAlphaOp | r3 stage; r4 op; r5 bias; r6 scale; r7 clamp; r8 output register | stage < 16 and output register < 4 |
| `0x80171FD0` | GXSetTevSwapMode | r3 stage; r4 raster selector; r5 texture selector | stage < 16 and each selector < 4 |
| `0x80171ED4` | GXSetTevKColor | r3 konstant color ID; r4 guest color pointer | ID < 4, before pointer access |
| `0x80171E10` | GXSetTevColor | r3 TEV register ID; r4 guest color pointer | ID < 4, before pointer access |
| `0x8017200C` | GXSetTevSwapModeTable | r3 table ID; r4-r7 red,green,blue,alpha channel selectors | table ID < 4 |

The `gx_tev.cpp` guards compare the original unsigned values, log an invalid
ID and return without invoking Aurora. They do not clamp or wrap it. They
also do not validate the color/alpha input enums, op/bias/scale enums or swap
table channel enums. A future bridge must preserve that distinction rather
than replacing all enum arguments with a common normalization rule.

`GXSetTevDirect` differs: the pinned wrapper has no `TevStageOk` call and
Aurora performs no array access for this particular operation. It forwards
to GXSetTevIndirect with indirect stage 0, format 8-bit, no bias, matrix off,
wrap off, both booleans false and alpha selection off. Do not accidentally
inherit the clamping of the separate GXSetTevIndirect guest wrapper: direct
calls the Aurora function, not that guest wrapper.

## Color pointers and frame behavior

GXSetTevColor and GXSetTevKColor each resolve exactly four guest bytes with
`Memory::GetPointer(cp, 4)` and construct a by-value `GXColor` in byte order
`r=p[0], g=p[1], b=p[2], a=p[3]`. No pointer is retained, no float or signed
color interpretation is involved and the guest bytes are not modified.
On little-endian Switch, loading a host `uint32_t` without endian conversion
would reverse these channels. Reading the four bytes directly or decoding
the existing big-endian Memory::Read32 contract yields the required RGBA.

For an invalid register/KColor ID, the pinned early return happens before
even resolving the pointer. For a valid ID, the pointer must provide the
full four-byte range. The pinned full memory implementation resolves a
missing range through its failure path; the Switch memory slice instead
returns nullptr for an unavailable or wrapped range. Copying the pinned
unchecked `p[0]` expression into a new Switch bridge would introduce a null
dereference. Any eventual adaptation needs the port's checked, diagnosed
four-byte read while retaining the ID-before-read order.

None of the nine wrappers calls EnsureAuroraFrameActive, writes a guest
GXData mirror or calls GxGuestWrite. The inspected Aurora functions and their
inline FIFO writes also contain no frame activation helper. No frame-start
call should be added merely by analogy with GXSetChanAmbColor. The active
frame in the preceding hardware snapshot is historical state, not a reason
to invent a new contract here.

## Aurora state and emitted commands

Each operation writes native BP commands using opcode `0x61` followed by a
32-bit register word and sets the host `__gx->bpSent` to 1. Native FIFO
writes go through `aurora::gx::fifo::write_*`, including display-list buffer
handling; they do not go through the port's instrumented GX_HLE_FIFO_Write
counter. An unchanged RMCP01 FIFO-write count therefore cannot measure this
new native work.

| Native operation | Host state / BP effect |
| --- | --- |
| Direct | Builds a zero indirect-control payload with BP address stage+`0x10`; forwards to GXSetTevIndirect |
| ColorIn | Updates `__gx->tevc[stage]`: four 4-bit fields at shifts 12,8,4,0; emits the complete cached word |
| AlphaIn | Updates `__gx->teva[stage]`: four 3-bit fields at shifts 13,10,7,4; emits the complete cached word |
| ColorOp / AlphaOp | Updates the matching tevc/teva cache: op low bit at 18, clamp at 19, destination register at 22; emits the complete cached word |
| SwapMode | Updates teva bits 0..3 for raster and texture swap selectors, preserving the other cached fields |
| Color | Emits RA and BG words at BP addresses `0xE0+2*id` and `0xE1+2*id`; places the RGBA8 components in the documented 11-bit fields |
| KColor | Emits the same address pair with 8-bit components and bit 23 set for the konstant-color bank |
| SwapModeTable | Updates `tevKsel[2*id]` bits 0..3 for red/green and `tevKsel[2*id+1]` bits 0..3 for blue/alpha, then emits both cached words |

For op values 0 or 1, ColorOp/AlphaOp set scale at bits 20..21 and bias at
16..17. Otherwise they use `(op >> 1) & 3` for the scale field and force
bias 3, which encodes the comparison form. Both paths set `op & 1`, the
boolean clamp and the output-register field. Forwarding the pinned arguments
to native GX preserves that branch; reducing every operation to ADD would
discard real semantics.

The native `SET_REG_FIELD` macro clears the destination field but **does not
mask the incoming value to that field's width** before shifting it. Out of
range, unguarded enum values can therefore affect neighboring bits. The
field widths above are not proof of safe truncation. Neither an added mask
nor an extra generic enum guard is established by this audit.

GXManage initializes the host shadow-register command bytes: tevc at
`0xC0+2*stage`, teva at `0xC1+2*stage`, and tevKsel at `0xF6+index`.
ColorIn/ColorOp share tevc; AlphaIn/AlphaOp/SwapMode share teva; swap tables
share tevKsel with konstant selectors. Recreating command words independently
in a bridge would risk losing both those register-address bytes and fields
set by earlier calls. Calling the pinned native function preserves these
host caches and display-list recording behavior. No additional guest mirror
or standalone bridge cache is justified.

Aurora ColorIn/AlphaIn/ColorOp/AlphaOp/SwapMode directly index the host arrays
without their own stage bound check. Color, KColor and SwapModeTable have
CHECK assertions, but CHECK expands to nothing with NDEBUG. The pinned
wrapper guards remain necessary for the guarded entries even in a release
build; relying on Aurora assertions would change that boundary.

## Static caller forecast and batch priorities

The local generated caller sets the number of TEV stages to 1, then still
initializes **all stage IDs 0..15**. An implementation must not reject stages
above the current active count or assume only stage 0 is used. For each
stage `s`, the static argument sequence is below. Only Direct stage 0 has an
arrival record; none of this loop is accepted as returned:

| Call | Forecast tuple in native argument order |
| --- | --- |
| Direct | `(s)` |
| Existing Order | `(s,255,255,255)` |
| ColorIn | `(s,15,15,15,15)` — four GX_CC_ZERO inputs |
| ColorOp | `(s,0,0,0,1,0)` — ADD, zero bias, scale 1, clamp true, PREV output |
| AlphaIn | `(s,7,7,7,7)` — four GX_CA_ZERO inputs |
| AlphaOp | `(s,0,0,0,1,0)` |
| SwapMode | `(s,0,0)` |

After that loop, the caller prepares KColor IDs 0..3 from a four-byte stack
slot at its new SP+20, then Color IDs 0..3 from a slot at SP+16. Their byte
values come from guest small-data memory and are **not captured or inferred**
here. The pointers are static expressions, not addresses observed in a
hardware report. Finally it prepares four SwapModeTable calls:
`(0,0,1,2,3)`, `(1,0,0,0,3)`, `(2,1,1,1,3)` and `(3,2,2,2,3)`.

The selected bounded batch covers the six per-stage scalar entries.
SwapModeTable follows the two pointer setters in the actual caller and stays
outside this batch until that preceding boundary is established. Preserve the distinct stage,
output-register and swap-selector guard rules and shared cache behavior. The
two pointer entries need a separate guest-memory boundary review, ID-first
ordering and independent synthetic RGBA cases. Their full legal ID sets are
0..3, not an active-count restriction or a single forecast ID.

This audit remains readiness work. Direct stage 0 is now the actual frontier,
but needs an attributable later return before acceptance. Static call order
does not establish which other entry will block next, that its whole loop
already executes, or that adding the bridges will produce recognizable game
images. The user confirmed a black screen. Snapshot 605367, before the
coordinate loop, records 1556 FIFO writes and 99 successful presents / 0
failures; it does not measure the later native work. The separate audit NRO
`b3484117` / `7ecbc8a9...` subsequently preserved this normal path on
hardware; see [the audit report](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md).
The six-setter implementation is now a [separate bounded candidate](GX_TEV_SCALAR_BATCH_2026-10-03.md)
with completed local gates/private build and a pending fresh hardware run. This neighbor audit itself provides static readiness evidence; the separate
candidate record owns implementation, build, test and hardware claims.
