# Hardware result — SZS decode crossed / GXInitTexObj frontier (2026-09-24)

Tracking: #117, #154, #162

## Result

The rendered real-Switch run after merged #229 hardware-validates the pinned
`EGG::Decomp::decodeSZS (0x80218C2C)` boundary and advances directly into
real texture-object initialization.

## decodeSZS is hardware-proven

The rendered-only decode status records:

```text
status       = decode-pass
src          = 0x94226C20
dst          = 0x80F10300
expand_size  = 2627200
src_consumed = 299969
dst_produced = 2627200
```

The source address is the same buffer populated by the successful local DVD
read of:

```text
/Boot/Strap/eu/English.szs
result = 299969
size   = 299969
```

This proves the complete boot-resource path through:

```text
local RMCP01 DVD read
  -> compressed English.szs in guest RAM
  -> pinned Yaz0/SZS decoder
  -> 2,627,200-byte decompressed resource in guest RAM
```

The decoder consumes exactly the 299,969-byte compressed file and produces the
entire expanded payload before execution advances.

## Previous scheduler and TaskThread fixes remain healthy

The AsyncDisplay VI idle recovery remains crossed:

```text
scheduler_pending = 0x02008000
default_state     = READY
default_priority  = 16
default_queue     = 0x80347830
```

The TaskThread continues to dispatch the real job:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

The receive slot also remains correct through output write, sender wakeup and
interrupt restore.

## Graphics invariants remain healthy

The rendered graphics report again reaches:

```text
PASS FIRST_RMCP01_FIFO_WORK
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

So the resource-path progression does not regress the hardware-proven
FIFO/Aurora/Dawn/NVK present chain.

## New exact blocker

The new durable blocker is:

```text
kind   = DIRECT
target = 0x801707F8
r3     = 0x901136B4
r4     = 0x80F103E0
r5     = 0x00000340
r6     = 0x000001C8
stage  = RMCP01_GX_FLUSH
```

RMCP01 maps `0x801707F8` exactly to:

```text
GXInitTexObj(
    GXTexObj* obj,
    void* image_ptr,
    u16 width,
    u16 height,
    GXTexFmt format,
    GXTexWrapMode wrap_s,
    GXTexWrapMode wrap_t,
    GXBool mipmap)
```

The first four live arguments already prove:

```text
obj       = 0x901136B4
image_ptr = 0x80F103E0
width     = 0x340 = 832
height    = 0x1C8 = 456
```

The image pointer lies inside the freshly decompressed resource beginning at
`0x80F10300`, only `0xE0` bytes after its base. This ties the texture
initialization directly to the resource that just crossed `decodeSZS`.

The blocker record does not capture r7-r10. The candidate therefore consumes
the live guest register values for format/wrapS/wrapT/mipmap instead of
fabricating them.

## Pinned WiiCompiled contract

Pinned WiiCompiled native-overrides this exact address. Its boundary:

1. canonicalizes the guest image address;
2. creates/updates an Aurora `GXTexObj` associated with the guest object;
3. calls Aurora `GXInitTexObj` with the live width/height/format/wrap/mipmap;
4. writes the SDK-format 32-byte `GXTexObj` representation into guest RAM,
   including image address, dimensions, format, block count/type and flags.

## Minimal candidate

Port only `GXInitTexObj (0x801707F8)`.

The Switch candidate:

- consumes live r3-r10;
- preserves a host `GXTexObj` keyed by the guest object address;
- forwards the exact live descriptor to Aurora on the rendered target;
- mirrors the pinned 32-byte guest `GXTexObj` layout;
- records rendered-only `fast-track-gx-init-tex-obj.txt` including the
  previously unobserved r7-r10 values and resulting guest words.

No `GXLoadTexObj`, `GXInitTexObjCI`, LOD, TLUT or neighboring texture
boundary is added before hardware reaches it.

## Next hardware acceptance

A PASS requires:

1. `fast-track-gx-init-tex-obj.txt` reports `init-pass`;
2. the descriptor shows the real hardware format/wrap/mipmap values;
3. execution durably progresses beyond `0x801707F8`;
4. the SZS decode remains `decode-pass`;
5. the existing TaskThread / VI idle / rendered-GPU invariants remain healthy;
6. the next distinct hardware blocker becomes the new frontier.
