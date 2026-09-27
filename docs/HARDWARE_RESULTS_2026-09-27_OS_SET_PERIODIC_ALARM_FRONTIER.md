# Hardware result — fourth GXInitTexObjLOD crossed / OSSetPeriodicAlarm frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged fourth
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x9018E480`
moves durably beyond that GX boundary.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x801A08E0
guest pc         : 0x800060A4
r1               : 0x80398BD8
r2               : 0x8038EFA0
r3               : 0x802D58B8
r4               : 0x00000000
r5               : 0x00000000
r6               : 0xEA6EDB94
r7               : 0x00000000
r8               : 0x00000000
r13              : 0x8038CC00
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

Runtime invariants remain healthy through the blocker:

- 48,375 translated / 47,769 post-main dispatches;
- 2,091 StaticR dispatches;
- 1,240 RMCP01 FIFO writes;
- 84 GXCopyDisp calls;
- 84 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs and StaticR.rel reads preserved;
- SZS decode PASS.

This proves the fourth exact `GXInitTexObjLOD` descriptor is
hardware-crossed.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x801A08E0` to `OSSetPeriodicAlarm`.

The matching MKW decomp exposes the original RVL contract:

1. disable guest interrupts;
2. write the 64-bit repeat interval at alarm + 0x18;
3. convert the supplied start time with the guest system-time adjustment at
   0x800030D8 and write alarm + 0x20;
4. insert the alarm into the guest alarm queue at r13 - 0x6360;
5. restore the prior interrupt state.

The hardware-observed call has:

```text
alarm      = 0x802D58B8
start      = 0x00000000EA6EDB94
period     = 0x0000000000000000
r13        = 0x8038CC00
queue base = 0x803868A0
```

The handler lives in guest r9 and is consumed from the live call. It was not
part of the generic blocker record and is therefore not hard-coded.

Pinned WiiCompiled ultimately reaches the RVL decrementer programming path when
an inserted alarm becomes queue head, but its host PPCMtdec implementation is
itself stubbed. The Switch candidate therefore keeps the guest queue/field
semantics only and does not invent a host timer or alarm callback pump.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x801A08E0>` with:

- exact OSAlarm field layout;
- Wii system-time adjustment;
- pinned periodic fire-time calculation;
- sorted guest alarm-queue insertion;
- existing Switch interrupt-state HLE.

Do not pre-port `OSSetAlarm`, `OSCancelAlarm`, decrementer callbacks, alarm
dispatch/pumping, or neighboring timer APIs until hardware requires them.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`OSSetPeriodicAlarm (0x801A08E0)` while preserving scheduler, resource,
FIFO, GXCopyDisp and GPU-present invariants.
