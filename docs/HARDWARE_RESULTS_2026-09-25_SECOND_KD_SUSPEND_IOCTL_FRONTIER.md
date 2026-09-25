# Hardware result: WPADSetSyncDeviceCallback crossed to second KD suspend ioctl

Date: 2026-09-25

## Hardware evidence

The rendered real-Switch run built after merged main
`65b59534dacce549906fbf19ac74b55a143895af` no longer stops at
`WPADSetSyncDeviceCallback (0x801BF640)`.

The durable blocker moves to PAL `IOS_Ioctl (0x80194290)` with:

```text
kind                  : IOS_IOCTL_KD_CMD2_UNPROVEN_ARGUMENTS
target                : 0x80194290
r3 / fd               : 0x000007D1 (2001)
r4 / cmd              : 0x00000001
r5 / inBuf            : 0x00000000
r6 / inLen            : 0x00000000
r7 / outBuf           : 0x80356F40
r8 / outLen           : 0x00000020
stage                 : HOST_CONTEXT_SWITCH_RETURNED
```

The IOS open telemetry immediately identifies fd 2001 as another exact:

```text
path=/dev/net/kd/request
mode=0
fd=2001
```

The previous fd-2000 command-2 boot probe and fd-2000 close remain crossed.

## Durable progress

The same run preserves and advances the prior invariants:

```text
dispatch count         : 50315+
post-main dispatch     : 49709+
StaticR dispatches     : 406+
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 3882+
OSWakeupThread hits    : 7773+
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

`English.szs` and `StaticR.rel` still read successfully, and the Strap SZS
decode still produces 2,627,200 bytes.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision, `IOS_Ioctl (0x80194290)` routes a
`/dev/net/kd/request` handle to `HandleKdIoctl`.

For `cmd=0x01` (NWC24 suspend scheduler), the complete pinned behavior is:

```cpp
WriteReturn(outBuf, outLen, 0);
return 0;
```

For the observed tuple this means:

1. write guest result word `0` at `0x80356F40`;
2. return IOS result `0` in guest `r3`;
3. no input is consumed;
4. no host socket/scheduler side effect is performed.

## Switch candidate

Accept only the exact hardware-observed second request tuple:

```text
fd     = 2001
cmd    = 1
inBuf  = 0
inLen  = 0
outBuf != 0
outLen = 0x20
```

The bridge writes only the first output word and returns zero. Any repeat,
argument variation, command 3, later command 2, close of fd 2001, ioctlv, or
other network operation remains a fresh hardware frontier.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond
`IOS_Ioctl (0x80194290)` for this exact fd-2001/cmd-1 call while preserving
the scheduler, FST, FIFO, GXCopyDisp and present invariants.
