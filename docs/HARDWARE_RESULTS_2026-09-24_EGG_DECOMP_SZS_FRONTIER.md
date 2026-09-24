# Hardware result — AsyncDisplay idle wake crossed / EGG decodeSZS frontier (2026-09-24)

Tracking: #117, #154, #162

## Result

The rendered real-Switch run after the VI-only SelectThread idle correction
hardware-validates the previous scheduler frontier and reaches a distinct game
resource boundary.

## AsyncDisplay idle wake is hardware-proven

At the old idle frontier, the default thread is still parked on the
AsyncDisplay synchronization queue:

```text
default_thread = 0x80347498
default_state  = WAITING
default_queue  = 0x804294A4
pending        = 0
```

The new idle-recovery telemetry then records:

```text
scheduler_pending = 0x02008000
default_state     = READY
default_priority  = 16
default_queue     = 0x80347830
os_current        = 0x803478B0
os_running        = 0
```

Execution subsequently resumes the default thread itself:

```text
guest fiber current = 0x80347498
OS current/running  = 0x80347498 / 0x80347498
default state       = RUNNING
default queue       = 0
```

This closes the full hardware path:

```text
AsyncDisplay::syncTick
  -> OSSleepThread(mSyncQueue)
  -> SelectThread idle
  -> VI retrace / PostRetraceCallback
  -> OSWakeupThread(mSyncQueue)
  -> default thread READY
  -> SelectThread selects default thread
  -> default thread resumes
```

## Graphics invariants recover

The run progresses back through the real rendered path:

```text
FIRST_RMCP01_FIFO_WORK
GXBegin hits          = 1
AsyncDisplay endRender= 1
GXCopyDisp calls      = 1
present successes     = 1
present failures      = 0
FIFO produced work    = YES
```

The earlier repeated-GXFlush hardware proof remains historical evidence; this
run reaches the next resource boundary before incrementing the GXFlush counter.

## TaskThread and DVD invariants remain healthy

The TaskThread still dispatches the valid job:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

The local DVD bridge again reads:

```text
/Boot/Strap/eu/English.szs
result = 299969
size   = 299969
buffer = 0x94226C20
```

## New exact blocker

The new durable blocker is:

```text
kind   = DIRECT
target = 0x80218C2C
r3     = 0x94226C20
r4     = 0x80F10300
r5     = 0
r6     = 0x00005544
stage  = RMCP01_GX_FLUSH
```

RMCP01 maps `0x80218C2C` exactly to:

```text
EGG::Decomp::decodeSZS(const u8* src, u8* dst)
```

The source pointer is exactly the buffer populated by the successful
`English.szs` DVD read. Therefore this is a direct continuation of the
hardware-proven boot-resource path, not a speculative neighboring decoder.

Pinned WiiCompiled native-overrides this exact address with a Yaz0/SZS decoder
that:

1. reads the expanded size from source bytes `+4..+7`;
2. starts compressed payload consumption at `src+16`;
3. expands literal and back-reference runs into the guest destination;
4. rejects a back-reference before already produced output;
5. rejects output overrun;
6. returns the expanded byte count.

## Minimal candidate

Port only pinned `EGG::Decomp::decodeSZS (0x80218C2C)`.

The candidate:

- uses only guest-memory reads/writes;
- preserves the pinned Yaz0 decoding algorithm;
- validates mapped source/destination ranges and malformed back-references;
- returns the expanded byte count in guest `r3`;
- records rendered-only `fast-track-szs-decode-status.txt`.

No neighboring `decodeASH`, `decodeASR`, generic resource loader, DVD,
scheduler, audio, input or GX boundary is added.

## Next hardware acceptance

A PASS requires:

1. `fast-track-szs-decode-status.txt` reports `decode-pass`;
2. execution durably progresses beyond `0x80218C2C`;
3. the TaskThread job and `English.szs` read remain healthy;
4. the VI idle recovery remains crossed;
5. the next distinct hardware blocker becomes the new frontier.
