# Hardware result: GXInitTexObjLOD crossed to WPADSetSyncDeviceCallback

Date: 2026-09-25

## Hardware evidence

A real-Switch rendered fast-track run built after merged main
`81d383bdd7dd2eda7dcab459a581026615000867` no longer stops at the previous
`GXInitTexObjLOD (0x80170A4C)` frontier. The durable unsupported dispatch moved
to:

```text
kind                  : DIRECT
target                : 0x801bf640
guest pc              : 0x800060a4
r1                    : 0x80399038
r2                    : 0x8038efa0
r3                    : 0x805230e0
r4                    : 0x00000000
r5                    : 0x00000000
r6                    : 0x901aa39c
r7                    : 0x00000000
r8                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_RETURNED
```

The same run preserves sustained translated/rendered progress:

```text
dispatch count        : 32285
post-main dispatch    : 31679
RKSystem::run hits    : 1
StaticR dispatches    : 1889
TaskThread::run hits  : 2
GXCopyDisp calls      : 84
present successes     : 84
present failures      : 0
RMCP01 FIFO writes    : 1240
FIFO produced work    : YES
FST structurally valid: YES
renderer initialized  : YES
renderer frame active : YES
```

Resource/bootstrap invariants also remain valid: `English.szs` and
`StaticR.rel` read successfully and the SZS decode completes.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that exact revision, PAL `0x801BF640` is
`WPADSetSyncDeviceCallback_HLE(uint32_t callback)`.

Its complete behavior is:

```cpp
const uint32_t previous = g_state.syncDeviceCallback;
g_state.syncDeviceCallback = callback;
return previous;
```

The observed first callback argument is therefore:

```text
callback = 0x805230E0
```

The pinned WPAD stub initializes `syncDeviceCallback` to zero, so the first
observed call returns `0` and stores `0x805230E0`.

This boundary does not invoke the callback, start/stop synchronization, probe
controllers, touch guest memory, or require Bluetooth/Joy-Con/Wii Remote
behavior.

## Switch candidate

Expose only `KnownNativeCpuCall<0x801BF640>` and mirror the pinned host-side
callback swap:

1. capture guest `r3` as the new callback;
2. read the previous stored callback;
3. store the new callback;
4. return the previous value in guest `r3`.

Do not pre-port `WPADStartSimpleSync`, `WPADStopSimpleSync`,
`WPADProbe`, or any neighboring WPAD behavior.

## Hardware acceptance

The candidate is hardware-crossed only if a later durable blocker/milestone
proves execution beyond `0x801BF640` while preserving scheduler, FST, FIFO,
rendering and present invariants.

If a neighboring WPAD sync API becomes the next blocker, treat that address and
its live arguments/state as a fresh hardware frontier.
