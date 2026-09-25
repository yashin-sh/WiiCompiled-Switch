# Hardware result — OSCancelThread crossed / GXInitTexObjLOD frontier (2026-09-25)

Tracking: #117, #154, #162

## Durable crossing

The rendered real-Switch run built from merged PR #246 progresses durably
beyond `OSCancelThread (0x801AA1D4)` to a distinct later GX blocker.

The strongest durable snapshot records:

```text
dispatch count        : 38262
post-main dispatch    : 37656
RKSystem::run hits    : 1
StaticR dispatches    : 421
TaskThread::run hits  : 3
AsyncDisplay endRender: 92
GXFlush hits          : 96
RMCP01 FIFO writes    : 1408
FIFO produced work    : YES
GXCopyDisp calls      : 92
present successes     : 92
present failures      : 0
```

The default guest fiber remains current and both OS current/running contexts are
`0x80347498`, while execution continues far beyond the canceled TaskThread.
This distinct later blocker proves the first exact OSCancelThread path crossed.

## Additional resource progress

The same run advances well beyond the previous boot-resource set:

- `/contents/HomeButton.arc` read-pass: 2,156,800 bytes;
- `/contents/HomeButtonSe.arc` read-pass: 370,880 bytes;
- `/hbm/homeBtn_ENG.szs` read-pass: 66,753 bytes;
- that SZS expands to 432,160 bytes at `0x90123B00`;
- `/hbm/SpeakerSe.arc` read-pass: 59,424 bytes;
- `/hbm/home.csv` read-pass: 3,610 bytes;
- `/hbm/config.txt` read-pass: 35 bytes;
- `/hbm/homeBtnIcon.tpl` read-pass: 6,336 bytes;
- `/hbm/HomeButtonSe.arc` read-pass: 367,542 bytes.

This is real Home Button/UI resource initialization, not fabricated success.

## New exact blocker

```text
kind   = DIRECT
target = 0x80170A4C
r3     = 0x9018E120
r4     = 1
r5     = 1
r6     = 0
r7     = 0
r8     = 0
stage  = RMCP01_GX_INIT_TEX_OBJ
```

Pinned WiiCompiled maps `0x80170A4C` exactly to:

```cpp
GX__InitTexObjLOD_80170a4c(
    uint32_t obj,
    uint32_t minFilter,
    uint32_t magFilter,
    float minLod,
    float maxLod,
    float lodBias,
    uint32_t biasClamp,
    uint32_t edgeLod,
    uint32_t maxAniso)
```

The immediately preceding `GXInitTexObj` status is:

```text
obj      = 0x9018E120
data     = 0x901532E0
width    = 64
height   = 64
format   = 0
wrap_s   = 1
wrap_t   = 1
mipmap   = 0
word0    = 0x00000095
word1    = 0x00000000
word2    = 0x0000FC3F
word3    = 0x0080A997
word5    = 0x00000000
blocks   = 64
flags    = 2
```

The integer LOD arguments are therefore known:
`minFilter=1`, `magFilter=1`, `biasClamp=0`, `edgeLod=0`,
`maxAniso=0`.

The three PPC scalar-float arguments in f1/f2/f3 are not present in the current
blocker record. Pinned WiiCompiled casts them to float and uses them directly,
so they must not be guessed from the non-mipmap texture descriptor.

## Diagnostic-only candidate

For exactly `GXInitTexObjLOD (0x80170A4C)`, the durable blocker now captures:

- the object address and readability;
- min/mag filter integer arguments;
- effective f1/f2/f3 float values as hexadecimal floating-point text;
- exact f32 bit patterns for minLod/maxLod/lodBias;
- biasClamp / edgeLod / maxAniso;
- all eight guest GXTexObj words before the LOD update.

No GXInitTexObjLOD bridge is installed yet.
No neighboring texture LOD/filter/wrap/TLUT API is pre-ported.

## Next hardware acceptance

Run the private rendered build and inspect `fast-track-dispatch-blocker.txt`
first. If the target remains `0x80170A4C`, the captured complete tuple defines
the only GXInitTexObjLOD path eligible for implementation.

Visual Mario Kart Wii pixel correctness remains separately unproven.
