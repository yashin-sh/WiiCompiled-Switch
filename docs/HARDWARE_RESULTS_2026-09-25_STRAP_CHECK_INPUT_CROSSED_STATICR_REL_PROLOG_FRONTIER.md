# Hardware result — StrapScene input crossed / StaticR RelProlog frontier (2026-09-25)

Tracking: #117, #154, #155, #162, #5

## Durable crossing

The rendered real-Switch run built from merged PR #241 progresses durably
beyond `StrapScene::CheckInput (0x800077C8)` and reaches a distinct later
indirect-call blocker.

The durable snapshot reaches:

```text
dispatch count        : 17800
post-main dispatch    : 17194
RKSystem::run hits    : 1
TaskThread::run hits  : 2
AsyncDisplay endRender: 62
GXFlush hits          : 61
RMCP01 FIFO writes    : 765
FIFO produced work    : YES
GXCopyDisp calls      : 61
present successes     : 61
present failures      : 0
```

Resource/scheduler invariants remain intact:

- FST: 64,224 bytes / 2,096 entries, structurally valid;
- `/Boot/Strap/eu/English.szs`: 299,969-byte read-pass;
- `/rel/StaticR.rel`: 4,903,876-byte read-pass into `0x805102E0`;
- renderer initialized and frame active.

This later distinct boundary proves the merged StrapScene input acceptance is
hardware-crossed. The return value `1` alone is not being used as proof.

## New exact blocker

```text
kind   = INDIRECT_CALL_MISS
target = 0x8055531C
r3     = 0x805102E0
r4     = 0x00000001
r5     = 0x809C4F90
r6     = 0x80F10248
r7     = 0xCC010000
r8     = 0x00000000
stage  = RMCP01_GX_FLUSH
```

The target lies inside the pinned StaticR executable range and the pinned
RMCP01 map identifies it exactly as:

```text
0x8055531C RelProlog
```

Only the observed module-base relation in `r3` is used by the Switch guard.
No ABI meaning is inferred for `r4-r8` from this blocker.

## Pinned WiiCompiled semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
registers a native winner at exactly `0x8055531C`:

```cpp
extern "C" void StaticRProlog_RecompModInit_8055531c(CpuContext* ctx) {
    RecompMod::RunMemoryInitializers();
    func_8055531C(ctx);
    RecompMod::RunPostRelInitializers();
}
```

The original translated `func_8055531C` is intentionally retained in the base
translation. The build-shard emitter excludes native winners from the generated
indirect-dispatch table, which explains this exact `INDIRECT_CALL_MISS`: the
private translated body exists, but the Switch port had not yet installed the
pinned native winner in its indirect-native seam.

For the current base RMCP01 Switch product there are no generated mod
data-patch registrants. Therefore the two RecompMod host initializer phases are
empty in this product; they are not guest REL-loading or relocation behavior.

## Exact candidate

The candidate:

1. recognizes only target `0x8055531C`;
2. requires the hardware-observed StaticR module base `r3=0x805102E0`;
3. routes both direct and indirect calls through the same Switch native bridge;
4. executes the already-generated private `func_8055531C(ctx)`;
5. does not fabricate REL sections, relocations, constructors, or success state;
6. does not pre-port `RelEpilog` (`0x80555368`) or
   `RelUnresolvedSection` (`0x805553B0`).

A different module base aborts as
`STATICR_REL_PROLOG_UNPROVEN_MODULE`.

Public Nintendo-data-free CI provides mapping/link coverage only and never
executes the private RMCP01 RelProlog body.

## StaticR dispatch counter note

The snapshot still reports `StaticR dispatches = 0`. That does not conflict
with this frontier: the counter increments only after a target is accepted by
the translated-dispatch path. This target currently stops at
`INDIRECT_CALL_MISS` before a successful StaticR dispatch can be recorded.

## Next hardware acceptance

The RelProlog candidate is crossed only if a future private rendered run
durably continues beyond `0x8055531C`, reaches a later milestone, exposes a
new blocker, or produces an attributable native exception.

Visual Mario Kart Wii pixel correctness remains a separate, unproven
milestone.
