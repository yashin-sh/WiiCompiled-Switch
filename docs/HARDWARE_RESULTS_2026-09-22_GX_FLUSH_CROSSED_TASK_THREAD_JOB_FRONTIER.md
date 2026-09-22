# Hardware result — GXFlush crossed / TaskThread job-dispatch frontier (2026-09-22)

Tracking: #117, #162

## Result

The rendered real-Switch RMCP01 run hardware-crosses the previous PAL
`GXFlush (0x8016E654)` frontier and preserves the first game-facing GPU
present milestone.

The durable translated snapshot records:

```text
GXFlush hits          : 23
GXCopyDisp calls      : 23
present successes     : 23
present failures      : 0
FIFO produced work    : YES
RMCP01 FIFO writes    : 506
```

The independent watchdog also remains ACTIVE through thousands of translated
dispatches after the first flush. Therefore this is stronger evidence than a
counter hit alone: execution durably progresses beyond `GXFlush`.

The renderer separately preserves:

```text
PASS FIRST_RMCP01_FIFO_WORK
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

The FST remains structurally valid at `0x97DC0000`, 64,224 bytes / 2,096
entries, and the renderer remains initialized.

## New durable blocker

The final blocker is no longer in GX/Aurora:

```text
kind             : INDIRECT_CALL_MISS
target           : 0x8042E458
guest pc         : 0x8024373C
r1               : 0x8042E458
r2               : 0x8038EFA0
r3               : 0x80210078
r4               : 0x8042E438
r5               : 0x00000001
r6               : 0x00000000
r13              : 0x8038CC00
stage            : HOST_CONTEXT_SWITCH_RETURNED
```

The thread lifecycle log identifies `0x8042E480` as the priority-24
`EGG::TaskThread` worker:

```text
thread    = 0x8042E480
entry     = 0x8024373C
arg       = 0x8042BBF0
stored_r1 = 0x8042E458
vtable    = 0x802A3F90
vt_run    = 0x80242D7C
```

The observed miss target is therefore exactly the worker's saved guest stack
pointer. It must **not** be treated as a translated/native function address.

## Narrow attribution

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements
`TaskThread_run_HLE_80242d7c` as:

1. take the current guest `r1`;
2. use `r1 - 0x20` as the blocking `OSReceiveMessage` output slot;
3. read the resulting job pointer;
4. read `callback`, `arg`, `token` and `onDone` from that job;
5. set only `r3 = arg` before the indirect callback dispatch.

The Switch bridge mirrors that contract.

The final blocker register shape matches this path exactly:

- `r1 = 0x8042E458`;
- `r4 = 0x8042E438 = r1 - 0x20`;
- `r5 = 1`, the blocking receive flag;
- `r3 = 0x80210078`, consistent with the callback argument slot surviving
  into the indirect dispatch.

This strongly localizes the frontier to the TaskThread job/continuation path,
but the current telemetry does **not** record the actual `job` pointer or its
fields. It is therefore not yet valid to claim that the job itself, the queue
message, or the HostContext restore is the corrupt component.

## Diagnostic change

Add behavior-neutral rendered/local telemetry only:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-task-thread-last-dispatch.txt
```

Immediately before a real TaskThread indirect `callback` or `onDone`
dispatch it records:

- dispatch kind;
- task-thread pointer;
- guest stack pointer;
- message output pointer;
- job pointer;
- callback;
- argument;
- token;
- onDone;
- actual indirect target;
- live `r1/r3/r4/r5`.

No queue, scheduler, HostContext, GX, DVD, resource, or callback semantics are
changed. No mapping is added for `0x8042E458`.

## Next hardware acceptance

After the five public CI gates and the private rendered build:

1. preserve `GXFlush > 0`, real FIFO work and successful presents;
2. reproduce or move beyond the current resource-worker frontier;
3. collect `fast-track-task-thread-last-dispatch.txt` together with the
   normal blocker/thread/liveness files;
4. if the diagnostic target matches the blocker, classify the exact
   `job/callback/arg/onDone` source before changing runtime behavior;
5. if a different durable blocker appears, follow that blocker instead.

The next behavioral correction must come from that hardware evidence.