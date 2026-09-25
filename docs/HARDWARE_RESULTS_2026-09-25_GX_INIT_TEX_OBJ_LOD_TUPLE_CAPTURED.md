# Hardware result — exact GXInitTexObjLOD tuple captured (2026-09-25)

Tracking: #117, #154, #162

## Latest durable run

The run built from merged diagnostic PR #247 reaches the same exact
`GXInitTexObjLOD (0x80170A4C)` blocker and captures the complete first
Home Button/UI LOD tuple.

```text
obj                 : 0x9018E120
min / mag filter    : 1 / 1
minLod              : +0.0f
maxLod              : +0.0f
lodBias             : +0.0f
min/max/bias bits   : 0 / 0 / 0
biasClamp           : 0
edgeLod             : 0
maxAniso            : 0

pre-LOD words:
  0/1 = 0x00000095 / 0x00000000
  2/3 = 0x0000FC3F / 0x0080A997
  4/5 = 0x00000000 / 0x00000000
  6/7 = 0x00000000 / 0x00400102
```

The strongest durable snapshot preserves 421 StaticR dispatches,
1,408 RMCP01 FIFO writes and 92 successful presents with zero failures.

## Pinned semantics

Pinned WiiCompiled `GX__InitTexObjLOD_80170a4c`:

1. resolves the host texture object for the guest GXTexObj;
2. clamps/sanitizes only non-finite/negative min/max LOD values;
3. stores the supplied filter/LOD/aniso metadata;
4. calls Aurora `GXInitTexObjLOD`;
5. mirrors the LOD/filter state into the guest GXTexObj.

For this exact tuple, the pinned guest update produces:

```text
word0: 0x00000095 -> 0x00000195
word1: 0x00000000 -> 0x00000000
```

That follows from:
- minFilter=GX_LINEAR -> hardware min-filter encoding 4;
- magFilter=GX_LINEAR;
- edgeLod=false -> inverted guest bit8 set;
- lodBias=0;
- maxAniso=GX_ANISO_1;
- biasClamp=false;
- minLod=maxLod=0.

## Exact Switch candidate

The candidate accepts only:

- guest object `0x9018E120`;
- all eight exact pre-LOD GXTexObj words above;
- minFilter=1 / magFilter=1;
- f1/f2/f3 effective f32 bit patterns all exactly `0x00000000`;
- biasClamp=0 / edgeLod=0 / maxAniso=0.

Rendered builds require the same host GXTexObj to already exist from the
immediately preceding exact `GXInitTexObj` call. The bridge then calls:

```cpp
GXInitTexObjLOD(
    hostObj,
    GX_LINEAR,
    GX_LINEAR,
    0.0f,
    0.0f,
    0.0f,
    GX_FALSE,
    GX_FALSE,
    GX_ANISO_1);
```

and mirrors the pinned guest state.

Any object, descriptor, floating-point bit pattern, filter or boolean/aniso
variation remains a fresh hardware-defined blocker.

No neighboring texture LOD/filter/wrap/TLUT API is pre-ported.

## Next hardware acceptance

The candidate is crossed only when a future private rendered run durably
continues beyond `0x80170A4C` to a distinct later blocker/milestone.
A `lod-pass` status alone is not crossing evidence.

Visual Mario Kart Wii pixel correctness remains separately unproven.
