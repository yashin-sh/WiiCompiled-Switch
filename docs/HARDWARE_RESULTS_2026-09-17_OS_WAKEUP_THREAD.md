# Hardware result: SCGetProductArea crossed into OSWakeupThread

Date: 2026-09-17
Tracking: #117

## Durable blocker

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x801aaaa4
guest pc              : 0x8024373c
r1                    : 0x901125f8
r2                    : 0x8038efa0
r3                    : 0x804294a4
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_ENTER
action                : abort after durable blocker record
```

## Result

This hardware run proves that the merged PAL `SCGetProductArea` (`0x801B23A0`) bridge was crossed. The next direct unsupported boundary is PAL `OSWakeupThread` (`0x801AAAA4`) with wait queue `0x804294A4`.

The stage `HOST_CONTEXT_SWITCH_ENTER` is significant: this is no longer a leaf getter boundary. Pinned WiiCompiled drains the supplied OSThreadQueue, marks live threads READY, links non-suspended threads back into the priority run queues, resumes matching guest fibers, marks scheduler pending state, and can immediately enter `SelectThread(0)`.

The Switch bridge reuses the already hardware-validated scheduler/HostContext path rather than introducing a second scheduling model.
