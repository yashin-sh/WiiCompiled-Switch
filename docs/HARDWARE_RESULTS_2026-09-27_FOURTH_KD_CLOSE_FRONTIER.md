# Hardware result: fourth KD resume crossed to fd-2003 close

Date: 2026-09-27

## Hardware evidence

A rendered real-Switch run built from merged main
`86af7cf5f15c656aba33806e862a5601fb5988c1` proves the exact fourth
`/dev/net/kd/request` command-3 resume request is crossed:

```text
status=cmd3-fourth-request-resume-pass
fd=2003
cmd=3
in=0x00000000/0x00000000
out=0x80356f40/0x00000020
```

The durable unsupported dispatch then moves to:

```text
kind   : IOS_CLOSE_KD_UNPROVEN_FD
target : 0x80193AD8
r3/fd  : 0x000007D3 (2003)
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

The close status file independently records:

```text
status=IOS_CLOSE_KD_UNPROVEN_FD
fd=2003
```

## Durable runtime snapshot

The run remains healthy through the new frontier:

```text
dispatch count         : 51746
post-main dispatch     : 51140
StaticR dispatches     : 411
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 4062
OSWakeupThread hits    : 8133
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

The independent watchdog remains ACTIVE through its final sample. No native
exception file is present.

This run followed the KD scheduler path before returning to the later GX
texture frontiers. That is an execution-order difference, not a graphics
regression.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

PAL `IOS_Close (0x80193AD8)` routes a valid network HLE descriptor through
`Network_HLE_Close(fd)`, which removes the device handle and returns IOS
result 0 without guest-memory mutation.

Hardware has already proven fd 2003 is the fourth live KD request handle and
that its exact command-3 resume call completed immediately before this close.

## Switch candidate

Accept only the exact fourth close sequence:

1. fd is exactly `2003`;
2. the fourth KD request is still marked open;
3. the hardware-proven fd-2003/cmd-3 resume request has completed;
4. clear the fourth-open state;
5. return IOS result `0` in guest `r3`.

The already proven fd-2000, fd-2001 and fd-2002 closes remain supported.

Do not add generic network-handle close semantics or later fds/commands until
hardware reaches them.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond
`IOS_Close (0x80193AD8)` for fd 2003 while preserving scheduler, FST, FIFO,
GXCopyDisp and present invariants.
