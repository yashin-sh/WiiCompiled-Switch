# PAL fd-2001 IOS_Close blocker — `0x80193AD8`

The 2026-09-26 rendered real-Switch run proves the second KD request's
fd-2001/cmd-1 suspend tuple and then stops on:

```text
target = 0x80193AD8
fd/r3  = 2001
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

The same run records:

```text
/dev/net/kd/request
mode = 0
fd   = 2001
```

and immediately before the close:

```text
cmd1-second-request-suspend-pass
fd=2001
cmd=1
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` routes this valid network fd
through `Network_HLE_Close`, which removes the device handle and returns
`0`.

Implement only this exact fd-2001 close after the already observed cmd-1 pass.
Do not generalize to arbitrary network fds or later opens.

Crossing proof requires a later durable hardware frontier beyond this close.
