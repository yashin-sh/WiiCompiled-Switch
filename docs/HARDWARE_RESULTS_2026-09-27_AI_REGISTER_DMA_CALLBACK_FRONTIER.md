# Hardware result — __AXOutInitDSP crossed / AIRegisterDMACallback frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged
`__AXOutInitDSP (0x801269BC)` bridge moves durably beyond that boundary.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x80123F88
guest pc         : 0x800060A4
r1               : 0x80398BF8
r2               : 0x8038EFA0
r3               : 0x80126898
r4               : 0x000005A0
r5               : 0x802F81A0
r6               : 0x803864D6
r13              : 0x8038CC00
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

Runtime invariants remain healthy through the blocker:

- 37,985 translated dispatches / 37,379 post-main dispatches;
- 2,069 StaticR dispatches;
- 1,240 RMCP01 FIFO writes;
- 84 GXCopyDisp calls;
- 84 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs and StaticR.rel reads preserved;
- SZS decode PASS.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x80123F88` to
`AIRegisterDMACallback_80123f88`.

Pinned semantics are exact and guest-visible:

1. read the old callback from guest global `0x80386480`;
2. store the incoming callback there;
3. return the old callback pointer.

The hardware-observed incoming callback is:

```text
0x80126898
```

Pinned host bookkeeping also mirrors that pointer in host AI state, but that
host state is not part of the guest-visible contract needed to cross this
frontier.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x80123F88>`.

Use the incoming guest `r3` as the new callback, update only guest address
`0x80386480`, and return the previous value in `r3`.

Do not pre-port `AIInitDMA`, `AIStartDMA`, DMA accessors, callback
execution, or any neighboring audio function until real hardware reaches it.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`AIRegisterDMACallback (0x80123F88)` while preserving scheduler, resource,
FIFO, GXCopyDisp and GPU-present invariants.

The separate third exact `GXInitTexObjWrapMode` tuple on
`obj=0x9018E140` remains a scheduler-dependent pending gate.
