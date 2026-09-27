# Hardware result — fd-2003 close crossed / AIInit frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run durably crosses the previously pending
fourth KD request close:

```text
status=close-fourth-pass
fd=2003
```

The next durable unsupported direct boundary is:

```text
kind             : DIRECT
target           : 0x801240B0
guest pc         : 0x800060A4
r3               : 0x00000000
fast-track stage : HOST_CONTEXT_SWITCH_RETURNED
```

The observed argument therefore makes this exact call `AIInit(0)`.

Runtime invariants remain healthy through the blocker:

- translated dispatch count: >52k;
- post-main dispatch count: >51k;
- RMCP01 FIFO writes: 1,240;
- GXCopyDisp calls: 84;
- successful GPU presents: 84;
- present failures: 0;
- FST structurally valid;
- renderer initialized and frame-active.

This scheduler ordering proves the fd-2003 close independently of the still
pending third exact `GXInitTexObjWrapMode` tuple on `obj=0x9018E140`.

## Pinned attribution

Pinned WiiCompiled
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`
maps PAL `0x801240B0` to `AIInit_801240b0` in
`runtime/src/hle/audio/audio.cpp`.

On first initialization, the pinned implementation publishes these guest-visible
AI globals:

```text
0x80386480 DMA callback          = 0
0x8038644C callback busy         = 0
0x8038647C callback stack switch = r3
0x80386448 AI initialized        = 1
```

It also attempts to initialize the desktop host audio backend. That host-side
backend action is not part of the guest-visible contract needed to cross this
Switch hardware frontier.

## Minimal Switch implementation

Add only `KnownNativeCpuCall<0x801240B0>`.

For the first call, when all four guest words are mapped:

1. preserve the incoming guest CPU registers;
2. zero DMA callback and callback-busy;
3. copy the observed `r3` stack-switch argument into the guest global;
4. set AI initialized to 1.

If AI is already initialized, leave the guest globals unchanged.

Do not initialize a Horizon audio backend here. Do not add `AICheckInit`,
`AIStartDMA`, DMA accessors, callback registration, or any neighboring AI
entry point until real hardware reaches it.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond
`AIInit (0x801240B0)` while preserving scheduler, FST/resource, FIFO,
GXCopyDisp and GPU-present invariants. Any next unsupported AI/GX/KD boundary
is a fresh hardware frontier.
