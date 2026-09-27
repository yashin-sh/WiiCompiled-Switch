# PAL fd-2003 IOS_Close blocker — `0x80193AD8`

The 2026-09-27 rendered real-Switch run proves the fourth KD request
fd-2003/cmd-3 resume tuple and then stops on:

```text
target = 0x80193AD8
fd/r3  = 2003
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

Immediately before the close, the same run records:

```text
status=cmd3-fourth-request-resume-pass
fd=2003
cmd=3
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` routes a valid network fd
through `Network_HLE_Close`, which removes the device handle and returns
`0`.

Implement only this exact fd-2003 close after the already observed cmd-3
resume pass. Do not generalize to arbitrary network fds or later KD requests.

Crossing proof requires a later durable hardware frontier beyond this close.
