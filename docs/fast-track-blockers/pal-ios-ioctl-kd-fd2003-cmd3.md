# PAL fourth KD resume blocker — `0x80194290`

The 2026-09-26 rendered real-Switch run proves the fd-2002 close and then
reaches:

```text
fd      = 2003
cmd     = 3
inBuf   = 0
inLen   = 0
outBuf  = 0x80356F40
outLen  = 0x20
```

The same run records fd 2003 as the fourth
`/dev/net/kd/request`, mode 0 handle.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps command 3 to NWC24
resume-scheduler. After the initial boot try-suspend probe it advances the
internal scheduler phase to PostResumeProbe, writes result 0 to the output
buffer, and returns IOS result 0.

Implement only this exact first fd-2003/cmd-3 tuple. Do not pre-port the
fd-2003 close, later cmd-2 probe, cmd 4, socket startup, ioctlv, or neighboring
KD behavior.

The merged GXInitTexObjWrapMode candidate remains separately pending a durable
hardware crossing; this scheduler interleaving reached KD cmd 3 first.

Crossing proof requires a later durable hardware frontier beyond this call.
