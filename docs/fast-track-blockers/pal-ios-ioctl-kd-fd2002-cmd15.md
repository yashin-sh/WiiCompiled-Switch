# PAL third KD generated-user-id blocker — `0x80194290`

The 2026-09-26 rendered real-Switch run proves the fd-2001 close and then
reaches:

```text
fd      = 2002
cmd     = 0x0F
inBuf   = 0
inLen   = 0
outBuf  = 0x80356F40
outLen  = 0x20
```

The same run records fd 2002 as a third
`/dev/net/kd/request`, mode 0 open.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps command 0x0F to the
generated NWC24 user-id request. Its reply for a sufficiently large output
buffer is:

```text
out+0x00: result = 0
out+0x04: user id = 0x000000014D4B5752 (u64)
out+0x0C: creation stage = 1 (Generated)
IOS return = 0
```

The output range is zero-filled before these fields are written.

Implement only the exact first observed fd-2002/cmd-0x0F tuple with no input
and outLen 0x20. Do not pre-port fd-2002 close, cmd 3, later cmd 2 phases,
ioctlv, or neighboring network behavior.

Crossing proof requires a later durable hardware frontier beyond this call.
