# PAL fd-2002 IOS_Close blocker — `0x80193AD8`

The 2026-09-26 rendered real-Switch run proves the third KD request's
fd-2002/cmd-0x0F generated-user-id tuple and then stops on:

```text
target = 0x80193AD8
fd/r3  = 2002
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

Immediately before the close, the same run records:

```text
status=cmd15-third-request-user-id-pass
fd=2002
cmd=15
```

and fd 2002 is the live `/dev/net/kd/request` handle.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` routes this valid network fd
through `Network_HLE_Close`, which removes the device handle and returns
`0`.

Implement only this exact fd-2002 close after the already observed cmd-0x0F
pass. Do not generalize to arbitrary network fds or later opens.

Crossing proof requires a later durable hardware frontier beyond this close.
