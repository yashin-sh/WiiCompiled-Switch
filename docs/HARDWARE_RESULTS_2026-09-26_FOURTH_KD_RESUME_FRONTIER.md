# Hardware result: third KD close crossed to fourth KD resume request

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`37f152181ab7961e64dc5405c74d1af94078dbfd` proves the third KD request
close is crossed:

```text
status=close-third-pass
fd=2002
```

The next durable blocker is PAL `IOS_Ioctl (0x80194290)`:

```text
kind                  : IOS_IOCTL_KD_UNPROVEN_TUPLE
target                : 0x80194290
r3 / fd               : 0x000007D3 (2003)
r4 / cmd              : 0x00000003
r5 / inBuf            : 0x00000000
r6 / inLen            : 0x00000000
r7 / outBuf           : 0x80356F40
r8 / outLen           : 0x00000020
stage                 : HOST_CONTEXT_SWITCH_RETURNED
```

The same run identifies fd 2003 as a fourth exact request handle:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2003
```

## Durable runtime snapshot

The run remains live and healthy through the blocker:

```text
dispatch count         : 50477
post-main dispatch     : 49871
StaticR dispatches     : 486
VIWaitForRetrace hits  : 175
PostRetrace cb hits    : 3830
OSWakeupThread hits    : 7669
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

The watchdog stays ACTIVE through the final sample. No native exception or
panic file is present.

This run did not reach the newly merged GXInitTexObjWrapMode bridge; no
`fast-track-gx-init-tex-obj-wrap-mode.txt` file is produced. That does not
invalidate the prior GX frontier: thread scheduling reached the fourth KD
request first in this run.

## Decomp attribution

Public `doldecomp/mkw` confirms `IOS_Ioctl` as the SDK IPC API used by
RMCP01. The command-specific NWC24 semantics are not encoded by the public
function name and therefore come from the pinned WiiCompiled implementation.

## Pinned attribution

Pinned upstream:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision, `HandleKdIoctl(cmd=0x03)` is the NWC24 resume-scheduler
request:

```cpp
case 0x03:
    if (g_kdBootProbeSeen &&
        g_kdTrySuspendPhase == KdTrySuspendPhase::Boot) {
        g_kdTrySuspendPhase = KdTrySuspendPhase::PostResumeProbe;
    }
    WriteReturn(outBuf, outLen, 0);
    return 0;
```

So the exact observed behavior is:

1. acknowledge the scheduler resume transition after the initial boot
   try-suspend probe;
2. write WC24 result `0` at `outBuf+0`;
3. return IOS result `0`.

There is no socket or host-network side effect.

## Switch candidate

Accept only the exact fourth request tuple:

```text
fd     = 2003
cmd    = 3
inBuf  = 0
inLen  = 0
outBuf != 0
outLen = 0x20
```

Require the already observed earlier KD initialization state, record the
resume transition, write result 0, and return zero.

Do not add fd-2003 close, the post-resume command-2 probe, cmd 4, socket
startup, ioctlv, or generic KD behavior.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond this exact
fd-2003/cmd-3 `IOS_Ioctl (0x80194290)` while preserving scheduler, resource,
FIFO, GXCopyDisp and present invariants.
