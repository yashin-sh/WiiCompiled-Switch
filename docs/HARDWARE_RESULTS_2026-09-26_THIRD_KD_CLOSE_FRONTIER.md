# Hardware result: third KD user-id crossed to fd-2002 close

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged runtime main
`1917eaceac9c1aa81cd80b63a86b5e63efdb4852` proves the exact third
`/dev/net/kd/request` command-0x0F generated-user-id tuple is crossed:

```text
status=cmd15-third-request-user-id-pass
fd=2002
cmd=15
in=0x00000000/0x00000000
out=0x80356f40/0x00000020
```

The durable unsupported dispatch then moves to:

```text
kind                  : IOS_CLOSE_KD_UNPROVEN_FD
target                : 0x80193AD8
r3 / fd               : 0x000007D2 (2002)
stage                 : HOST_CONTEXT_SWITCH_RETURNED
```

The same run records the active request handle as:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2002
```

## Durable runtime snapshot

The run remains healthy through the new frontier:

```text
dispatch count         : 42540
post-main dispatch     : 41934
StaticR dispatches     : 405
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 3419
OSWakeupThread hits    : 6847
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

`English.szs` and `StaticR.rel` reads remain successful. No evidence in
this run points to a graphics regression.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision, PAL `IOS_Close (0x80193AD8)` routes network HLE file
descriptors to `Network_HLE_Close(fd)`:

```cpp
if (!GetDeviceKind(fd)) {
    return -101;
}
RemoveDevice(fd);
return 0;
```

Hardware has already proven fd 2002 is the third live KD request handle and
that its exact command-0x0F generated-user-id call completed immediately
before this close.

## Switch candidate

Accept only the exact third close sequence:

1. fd is exactly `2002`;
2. the third KD request is still marked open;
3. the hardware-proven fd-2002/cmd-0x0F request has completed;
4. clear the third-open state;
5. return IOS result `0` in guest `r3`.

The already proven fd-2000 and fd-2001 closes remain supported.

Do not add generic network-handle close semantics or support later fds until
hardware reaches them.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond
`IOS_Close (0x80193AD8)` for fd 2002 while preserving scheduler, FST, FIFO,
GXCopyDisp and present invariants.
