# PAL EGG::TaskThread::run — 0x80242D7C

Tracking: #117, #154

## Hardware blocker

After #191 crosses PAL `GXDrawDone`, the next unsupported boundary is a
virtual indirect jump from `EGG::Thread::start(void*)`:

```text
kind   : INDIRECT_JUMP_MISS
target : 0x80242D7C
pc     : 0x8024373C
r3     : 0x8042BBF0
stage  : HOST_CONTEXT_SWITCH_ENTER
```

The matching thread event identifies OSThread `0x8042E480`, priority 24, with
vtable `0x802A3F90` and virtual `run()` slot `0x80242D7C`.

## Pinned attribution

At WiiCompiled pin
`a135beb201042b20f390c6695ca6b26768820fb4`, this address is the native
override `TaskThread_run_HLE_80242d7c`.

The pinned implementation blocks on the embedded message queue, executes queued
job callbacks, sends optional completion tokens, clears retired job slots, and
loops. It also seeds GQR2-GQR5 and carries the pin's THP prepare special case.

Later hardware telemetry supersedes the earlier ResourceManager attribution
for this exact object. The observed worker has `mJobCount=5` and
`mStackSize=0x2800`; the currently decompiled ResourceManager TaskThread uses
a different allocation shape. Treat this object only as the priority-24
`EGG::TaskThread` worker until its owner is proven.

## Switch implementation

The Switch bridge mirrors the pinned worker loop and reuses hardware-proven
`OSReceiveMessage` / `OSSendMessage`. Dynamic job callbacks and completion
callbacks continue through the generated indirect dispatcher.

Because this target is reached through a vtable, the Switch indirect resolver
handles the pinned native target before searching the generated translated
table.

The run now hardware-crosses `0x80242D7C`. Follow-up telemetry proves
`TaskThread::request` sends the valid `mJobs[0]` pointer `0x8042E7DC`,
but the worker later reads a stack-shaped value from the blocking receive output
slot. The current acceptance gate is to identify the exact
`OSReceiveMessage` phase that clobbers that slot; no function mapping or queue
behavior change is justified before that evidence.
