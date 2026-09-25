# Hardware result — GXSetTexCoordGen2 crossed / StrapScene::CheckInput frontier (2026-09-25)

Tracking: #117, #154, #162, #5

## Durable crossing

The real-Switch rendered run built from merged PR #239 progresses far beyond
the prior GXSetTexCoordGen2 boundary.

The durable post-main snapshot reaches:

```text
dispatch count        : 19718
post-main dispatch    : 19112
GXSetTexCoordGen2     : crossed by later durable work
GXFlush hits          : 60
GXCopyDisp calls      : 60
present successes     : 60
present failures      : 0
```

The same run preserves:

- RKSystem::run = 1;
- TaskThread::run = 2;
- /Boot/Strap/eu/English.szs read-pass at 299,969 bytes;
- /rel/StaticR.rel read-pass at 4,903,876 bytes;
- FST valid at 64,224 bytes / 2,096 entries;
- 758 RMCP01 FIFO writes with produced work;
- renderer initialized and frame active.

This proves `GXSetTexCoordGen2 (0x8016E37C)` is hardware-crossed.

## New exact blocker

```text
kind   = DIRECT
target = 0x800077C8
r3     = 0x90112A34
r4     = 0x90118B1C
r5     = 0x00000078
stage  = RMCP01_GX_FLUSH
```

Only r3 is an ABI argument for this function. r4+ are not interpreted as
StrapScene::CheckInput arguments.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x800077C8` exactly to:

```cpp
extern "C" uint32_t StrapScene__CheckInput_Skip(uint32_t scenePtr)
{
    (void)scenePtr;
    settings_overlay::NotifyStrapInputAccepted();
    return 1;
}
```

The upstream comment states that StrapScene::calc reaches this check only after
the scene loading/timing gates are complete. The HLE accepts that check without
requiring guest controller hardware.

## Exact candidate

The Switch candidate accepts only the hardware-observed
`scenePtr=0x90112A34`.

It reproduces only the guest-visible contract:

```text
r3 = 1
```

The desktop-only settings-overlay notification is intentionally omitted because
that host overlay does not exist on Switch and has no guest-visible state.

A different scene pointer aborts as `STRAP_CHECK_INPUT_UNPROVEN_SCENE` and
becomes a fresh hardware-defined frontier.

No Joy-Con/Wii controller mapping, neighboring StrapScene behavior, scene
transition, input API, audio API, or scheduler behavior is added.

## Next acceptance

A future run must durably progress beyond `0x800077C8` to count this
candidate as crossed. A later blocker, milestone, or attributable native
exception defines the next step.
