# Hardware result — IOS_Open /dev/net/kd/request frontier (2026-09-24)

Tracking: #117, #154, #162

## Hardware evidence

The rendered run after merged #231 identifies the exact IOS request:

```text
DIRECT 0x801938F8
r3 = 0x802A2160
r4 = 0
ios open path = /dev/net/kd/request
ios open mode = 0
stage = HOST_CONTEXT_SWITCH_RETURNED
```

The same hardware run also validates the newly added GXInitTexObj bridge:

```text
status = init-pass
width  = 832
height = 456
format = 4
mipmap = 0
```

The FST, English.szs DVD read, SZS decode, AsyncDisplay VI-idle recovery and
previously proven real FIFO/GPU-present path remain healthy.

## Pinned behavior

Pinned WiiCompiled maps `0x801938F8` to `NAND_IOS_Open_HLE(pathPtr, mode)`.
For `/dev/net/kd/request`, `Network_HLE_OpenDevice` classifies the device
as `KdRequest`. Networking is enabled by default and device handles are
allocated monotonically starting at fd 2000.

Opening this device alone does not create a socket or perform host network I/O.

## Minimal correction

Port only this exact hardware-proven open request:

- accept `/dev/net/kd/request`;
- require the observed mode `0`;
- return the pinned first network-device fd sequence starting at 2000;
- emit rendered-only `fast-track-ios-open-kd-request.txt`;
- abort as a fresh frontier for any other path or mode.

No IOS ioctl/ioctlv/close, KD command semantics, NCD, IP, SSL, DNS or socket
behavior is pre-ported.

## Hardware acceptance

1. `fast-track-ios-open-kd-request.txt` reports `open-pass`;
2. first returned fd is 2000;
3. execution durably progresses beyond `0x801938F8`;
4. existing resource/scheduler/render invariants remain healthy;
5. the next exact IOS/network/application blocker becomes the new frontier.

## Implementation state

PR #232 implements the minimal correction above and is squash-merged on
`main` as:

```text
4f1d0188c61d0e12267e5468c1b6591df1d29a4d
```

Its exact PR head passed all five required public CI workflows:
`lint`, `fast-track-startup`, `bootstrap-register-prelude`,
`stateful-translated-sequence`, and `build-switch`.

This does **not** upgrade the boundary to hardware-crossed. The next private
rendered build and real-Switch run must still prove `open-pass`, first
`fd=2000`, and durable progression beyond `0x801938F8`.
