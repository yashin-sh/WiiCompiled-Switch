# Hardware result: second KD suspend crossed to fd-2001 close

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`e889833f342420a769ec4ea35e90ee98cdbfcb66` proves the exact second
`/dev/net/kd/request` command-1 suspend tuple is crossed:

```text
status=cmd1-second-request-suspend-pass
fd=2001
cmd=1
in=0x00000000/0x00000000
out=0x80356f40/0x00000020
```

The durable unsupported dispatch then moves to:

```text
kind                  : IOS_CLOSE_KD_UNPROVEN_FD
target                : 0x80193AD8
r3 / fd               : 0x000007D1 (2001)
stage                 : HOST_CONTEXT_SWITCH_RETURNED
```

The same run records the active request handle as:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2001
```

## Durable progress

The run remains healthy and advances beyond the previous frontier:

```text
dispatch count         : 51460
post-main dispatch     : 50854
StaticR dispatches     : 484
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 4000
OSWakeupThread hits    : 8009
GXBegin hits           : 126
GXFlush hits           : 88
RMCP01 FIFO writes     : 1240
GXCopyDisp calls       : 84
present successes      : 84
present failures       : 0
FST structurally valid : YES
renderer initialized   : YES
renderer frame active  : YES
```

The watchdog remains ACTIVE through the run, and no native exception is
recorded. `English.szs` and `StaticR.rel` still read successfully.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision, PAL `IOS_Close (0x80193AD8)` checks whether the fd belongs
to the network HLE and then calls `Network_HLE_Close(fd)`.

For a valid network-device handle, the pinned close semantics are:

```cpp
if (!GetDeviceKind(fd)) {
    return -101;
}
RemoveDevice(fd);
return 0;
```

Hardware has already proven fd 2001 is the second live KD request handle and
that its exact command-1 suspend call completed immediately before this close.

## Switch candidate

Accept only the exact second close sequence:

1. fd is exactly `2001`;
2. the second KD request is still marked open;
3. the hardware-proven fd-2001/cmd-1 suspend call has completed;
4. clear the second-open state;
5. return IOS result `0` in guest `r3`.

The already proven fd-2000 close remains supported.

Do not add generic network-handle close semantics or support any later fd until
hardware reaches it.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond
`IOS_Close (0x80193AD8)` for fd 2001 while preserving scheduler, FST, FIFO,
GXCopyDisp and present invariants.
