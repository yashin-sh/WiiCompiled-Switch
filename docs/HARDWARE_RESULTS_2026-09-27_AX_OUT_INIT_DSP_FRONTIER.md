# Hardware result — AIInit crossed / __AXOutInitDSP frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged AIInit bridge moves
durably beyond PAL `AIInit (0x801240B0)`.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x801269BC
guest pc         : 0x800060A4
r1               : 0x80398BF8
r2               : 0x8038EFA0
r3               : 0x802F7480
r4               : 0x000005A0
r5               : 0x802F81A0
r6               : 0x803864D6
r13              : 0x8038CC00
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

Runtime invariants remain healthy through the blocker:

- >49k translated dispatches and >49k post-main dispatches;
- 2,089 StaticR dispatches in the latest durable post-main record;
- 1,240 RMCP01 FIFO writes;
- 84 GXCopyDisp calls;
- 84 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs and StaticR.rel reads preserved;
- SZS decode still passes;
- no native exception record is present.

The same run still records:

```text
status=close-fourth-pass
fd=2003
```

so the KD path remains crossed through fd 2003 close.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x801269BC` to `__AXOutInitDSP_801269bc` in
`runtime/src/hle/audio/audio.cpp`.

That wrapper calls `AxDspHle::InitForAXOut(ctx)`.

The pinned implementation initializes desktop host AX/DSP state, but its
guest-visible contract is deterministic:

- mark DSP initialized at `0x80386608`;
- clear DSP assert/task globals;
- populate the AX DSP task at `0x802F81A0`;
- copy three SDA halfwords derived from guest `r13`;
- publish the task as current/first/running;
- publish two AX-out SDA state words.

The exact static task constants are:

```text
task + 0x0C = 0x8027F820
task + 0x18 = 0x802F8200
task + 0x1C = 64
task + 0x20 = 3282
task + 0x28 = 0x80126948
task + 0x2C = 0x80126954
task + 0x30 = 0x801269A8
task + 0x34 = 0x801269B8
```

The host mix worker, host mail queue and audio backend are not guest memory and
are not required to represent this exact observed boundary.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x801269BC>`.

Mirror the pinned guest DSP task/global writes when the required guest ranges
are mapped. Preserve guest CPU registers and do not start a host audio worker.

Do not pre-port adjacent DSP mail, DMA, callback or AX functions. Any such
function must be reached on real hardware before support is added.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`__AXOutInitDSP (0x801269BC)` while preserving scheduler, resource, FIFO,
GXCopyDisp and GPU-present invariants.

The separate third exact `GXInitTexObjWrapMode` tuple on
`obj=0x9018E140` remains a scheduler-dependent pending gate.
