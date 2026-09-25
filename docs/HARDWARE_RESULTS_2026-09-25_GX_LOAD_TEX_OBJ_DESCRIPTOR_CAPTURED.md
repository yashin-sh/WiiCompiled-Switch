# Hardware result — GXLoadTexObj descriptor captured (2026-09-25)

Tracking: #117, #154, #162

## Reproduced exact blocker

The diagnostics-only build reproduces:

```text
kind   = DIRECT
target = 0x80170F2C
r3     = 0x901136B4
r4     = 0x00000000
stage  = RMCP01_GX_SET_CHAN_CTRL
```

Pinned WiiCompiled maps this exactly to:

```cpp
GX__LoadTexObj_80170f2c(uint32_t oa, uint32_t tid)
```

so the live call is `GXLoadTexObj(0x901136B4, 0)`.

## Exact 32-byte guest descriptor

The new blocker diagnostics report the complete GXTexObj as readable:

```text
word0 = 0x00000090
word1 = 0x00000000
word2 = 0x00471F3F
word3 = 0x0007881F
word4 = 0x00000000
word5 = 0x00000004
word6 = 0x00000000
word7 = 0x5CA00202
```

Following the pinned `ExtractTexObjMetaFromGuest` contract:

```text
data      = 0x00F103E0
width     = 832
height    = 456
format    = 4
format w2 = 4
wrap S/T  = 0 / 0
mipmap    = 0
```

The format sources agree. This is a non-CI texture, so the observed load does
not require a TLUT path.

## Payload size

Format 4 uses 4x4 RGBA8 blocks:

```text
blocksX = 832 / 4 = 208
blocksY = ceil(456 / 4) = 114
blocks  = 208 * 114 = 23,712
bytes   = 23,712 * 64 = 1,517,568 = 0x172800
```

The block count matches the descriptor's high halfword at +0x1C (`0x5CA0`).

## Preserved prior milestones

The same hardware run preserves:

- KD open fd 2000;
- first KD command-2 Boot probe;
- fd-2000 IOS_Close close-pass;
- FST 64,224 bytes / 2,096 entries;
- `English.szs` read-pass at 299,969 bytes;
- SZS output 2,627,200 bytes;
- `/rel/StaticR.rel` read-pass at 4,903,876 bytes;
- `RKSystem::run hits = 1`;
- `TaskThread::run hits = 2`;
- 29 RMCP01 FIFO writes with produced work;
- `GXCopyDisp calls = 1`;
- one successful present and zero failures.

Visual Mario Kart Wii pixel correctness remains unclaimed.

## Exact candidate

The candidate accepts only the exact observed call and descriptor:

- oa `0x901136B4`;
- tid 0;
- all eight descriptor words above unchanged;
- full `0x172800` backing range mapped.

For that one case it:

1. resolves the live guest backing `0x00F103E0`;
2. reconstructs an Aurora GXTexObj as 832x456 format 4, clamp/clamp,
   non-mipmapped;
3. applies the pinned-decoded linear/linear zero-LOD state;
4. binds the object to texture map 0 with `GXLoadTexObj`;
5. mirrors pinned guest GXData post-load state:
   `GXData+0x5FC |= 1` and `GXData+2 = 0`;
6. writes rendered-only `fast-track-gx-load-tex-obj.txt` with
   `status=load-pass`.

Every descriptor variation aborts as
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR` at the same exact native boundary.

No CI/TLUT API, neighboring LOD function, texture invalidation path, or generic
GXLoadTexObj implementation is added.

## Next hardware acceptance

After merge with all five required public CI gates green, the private rendered
run must not call this frontier crossed merely because
`fast-track-gx-load-tex-obj.txt` says `load-pass`.

Acceptance requires durable execution beyond `0x80170F2C`, a distinct later
blocker/milestone, or an attributable native exception. If another
GXLoadTexObj call arrives with different descriptor data, that exact variation
becomes the next frontier.
