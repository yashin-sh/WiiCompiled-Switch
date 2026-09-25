# Hardware result — GXSetTexCoordGen2 crossed / StrapScene::CheckInput frontier (2026-09-25)

Tracking: #117, #154, #162

## Durable crossing

The latest rendered real-Switch run proves the exact
`GXSetTexCoordGen2 (0x8016E37C)` tuple is crossed. The run advances to:

```text
dispatch count        : 19718
post-main dispatch    : 19112
AsyncDisplay endRender: 61
GXFlush hits          : 60
RMCP01 FIFO writes    : 758
GXCopyDisp calls      : 60
present successes     : 60
present failures      : 0
```

This is durable progression, not a single status hit.

## Preserved runtime/resource invariants

The same run keeps:

- FST published at `0x97DC0000`, 64,224 bytes / 2,096 entries;
- `/Boot/Strap/eu/English.szs` read-pass at 299,969 bytes;
- SZS output 2,627,200 bytes;
- `/rel/StaticR.rel` read-pass at 4,903,876 bytes;
- KD request open fd 2000;
- first KD command-2 Boot probe;
- fd-2000 close-pass;
- `RKSystem::run hits = 1`;
- `TaskThread::run hits = 2`;
- renderer initialized and frame active.

The repeated GX state counters also rise substantially: projection, viewport,
scissor, position-matrix, current-matrix, clear-vtx-desc, texture-gen count,
TEV state and related boundaries all repeat dozens of times.

## New exact blocker

```text
kind   = DIRECT
target = 0x800077C8
r3     = 0x90112A34
r4     = 0x90118B1C
r5     = 0x00000078
r6     = 0x80245816
r7     = 0xCC010000
r8     = 0x00000061
stage  = RMCP01_GX_FLUSH
```

Only `r3` is an ABI argument for this function. `r4-r8` are not interpreted
as StrapScene arguments.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x800077C8` exactly to:

```cpp
uint32_t StrapScene__CheckInput_Skip(uint32_t scenePtr)
{
    (void)scenePtr;
    settings_overlay::NotifyStrapInputAccepted();
    return 1;
}
```

The pinned implementation deliberately ignores `scenePtr`. The overlay
notification affects only the desktop startup/ImGui overlay timing; it is not
guest state.

## Exact candidate

The Switch candidate mirrors only the guest-visible result:

```cpp
cpu->gpr[3] = 1;
```

No desktop settings overlay is imported. No PAD, Wii Remote, controller
mapping, or neighboring StrapScene function is added.

## Next hardware acceptance

The boundary counts as crossed only if the next private rendered run durably
continues beyond `0x800077C8`, reaches a later milestone, or exposes a new
attributable blocker/exception.
