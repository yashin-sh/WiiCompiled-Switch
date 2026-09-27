# Hardware result — AIRegisterDMACallback crossed / AIInitDMA frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run durably crosses
`AIRegisterDMACallback (0x80123F88)`.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x80123FCC
guest pc         : 0x800060A4
r3               : 0x802F7D20
r4               : 0x00000180
r13              : 0x8038CC00
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

Runtime invariants remain healthy:

- 49,583 translated / 48,977 post-main dispatches;
- 2,089 StaticR dispatches;
- 1,240 RMCP01 FIFO writes;
- 84 GXCopyDisp calls;
- 84 successful presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs / StaticR.rel reads and SZS decode preserved.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x80123FCC` to `AIInitDMA_80123fcc`.

Pinned semantics update only shared AI DMA state:

```text
startAddr         = start
registerStartAddr = start & 0x1FFFFFE0
length            = length & 0x000FFFE0
bytesLeft         = encoded length
```

For this run:

```text
start             = 0x802F7D20
registerStartAddr = 0x002F7D20
length            = 0x00000180
bytesLeft         = 0x00000180
```

No guest memory is written and no audio backend is started.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x80123FCC>` and preserve this shared DMA state.

Do not pre-port `AIStartDMA`, DMA getters, callback execution, or neighboring
audio functions until hardware reaches them.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`AIInitDMA (0x80123FCC)` while preserving scheduler, resource, FIFO,
GXCopyDisp and GPU-present invariants.
