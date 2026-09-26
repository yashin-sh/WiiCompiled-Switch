# Hardware result: fd-2001 close crossed to third KD user-id request

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`3cd05a0ee88dba9e11410b20751a5c2de0173ebd` proves the second KD request
close is crossed:

```text
status=close-second-pass
fd=2001
```

The next durable blocker is PAL `IOS_Ioctl (0x80194290)`:

```text
kind                  : IOS_IOCTL_KD_UNPROVEN_TUPLE
target                : 0x80194290
r3 / fd               : 0x000007D2 (2002)
r4 / cmd              : 0x0000000F
r5 / inBuf            : 0x00000000
r6 / inLen            : 0x00000000
r7 / outBuf           : 0x80356F40
r8 / outLen           : 0x00000020
stage                 : HOST_CONTEXT_SWITCH_RETURNED
```

The same run identifies fd 2002 as a third exact request handle:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2002
```

## Preserved runtime invariants

The run remains live through the new frontier:

```text
dispatch count         : 49646
post-main dispatch     : 49040
StaticR dispatches     : 406
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 3754
OSWakeupThread hits    : 7517
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

The watchdog remains ACTIVE through the final sample. `English.szs` and
`StaticR.rel` still read successfully.

The lower heartbeat count than the prior run is not treated as a regression:
the durable frontier itself moved beyond the previously unsupported fd-2001
close.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision, `HandleKdIoctl(cmd=0x0F)` implements the generated NWC24
user-id request. For an output buffer of at least 0x10 bytes it:

1. zeroes the output range;
2. writes result word `0` at `out+0`;
3. writes stable user id `0x000000014D4B5752` at `out+4`;
4. writes `NWC24CreationStage::Generated (1)` at `out+0x0C`;
5. returns IOS result `0`.

## Switch candidate

Accept only the exact hardware-observed third request tuple:

```text
fd     = 2002
cmd    = 0x0F
inBuf  = 0
inLen  = 0
outBuf = 0x80356F40 (live value; any valid mapped address is accepted)
outLen = 0x20
```

The bridge mirrors the pinned reply shape and returns zero.

No fd-2002 close, command 3, later command 2, ioctlv, or generic KD behavior is
added.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond this exact
fd-2002/cmd-0x0F `IOS_Ioctl (0x80194290)` while preserving scheduler, FST,
FIFO, GXCopyDisp and present invariants.
