# PAL second KD suspend ioctl blocker — `0x80194290`

The 2026-09-25 rendered real-Switch run after merged
`WPADSetSyncDeviceCallback` support reaches a later `IOS_Ioctl` tuple:

```text
fd      = 2001
cmd     = 1
inBuf   = 0x00000000
inLen   = 0
outBuf  = 0x80356F40
outLen  = 0x20
```

The same run records fd 2001 as a second
`/dev/net/kd/request`, mode 0 open.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` routes command 1 to NWC24
suspend scheduler and performs only:

```text
out[0] = 0
IOS return = 0
```

No input buffer, socket operation, host scheduler transition, or network access
is involved.

Implement only this exact first observed fd-2001/cmd-1 tuple. Do not pre-port
command 3, later command 2 phases, fd-2001 close, ioctlv, or neighboring network
behavior.

Crossing proof requires a later durable hardware frontier beyond this call.
