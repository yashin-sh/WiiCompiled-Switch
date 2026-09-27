# Hardware result — AIInitDMA crossed / AIStartDMA frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged
`AIInitDMA (0x80123FCC)` bridge moves durably beyond that boundary.

The new exact unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x80124048
guest pc         : 0x800060A4
r1               : 0x80398BF8
r2               : 0x8038EFA0
r3               : 0x802F7D20
r4               : 0x00000180
r13              : 0x8038CC00
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

Runtime invariants remain healthy through the blocker:

- 55,653 translated dispatches / 55,047 post-main dispatches;
- 2,168 StaticR dispatches;
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
maps PAL `0x80124048` to `AIStartDMA_80124048`.

Pinned semantics at this exact boundary are:

```text
sampleRate = 32000
enabled    = true
bytesLeft  = length
```

The pinned desktop runtime also calls its host audio backend initializer at
32 kHz. That desktop backend side effect is not a guest-visible requirement
for crossing this Switch hardware frontier.

The DMA state entering this boundary is already proven from the immediately
preceding AIInitDMA call:

```text
startAddr         = 0x802F7D20
registerStartAddr = 0x002F7D20
length            = 0x00000180
bytesLeft         = 0x00000180
```

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x80124048>`.

Set the shared audio state to 32 kHz, enabled, and reload bytesLeft from the
encoded DMA length. Do not construct a Horizon audio backend, consume guest DMA
memory, run the registered callback, or pre-port DMA getters.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`AIStartDMA (0x80124048)` while preserving scheduler, resource, FIFO,
GXCopyDisp and GPU-present invariants.

Any next AI getter, callback or DSP boundary is a fresh hardware frontier.
