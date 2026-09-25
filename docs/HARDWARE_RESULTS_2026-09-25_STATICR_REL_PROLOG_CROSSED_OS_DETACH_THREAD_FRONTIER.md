# Hardware result — StaticR RelProlog crossed / OSDetachThread frontier (2026-09-25)

Tracking: #117, #155, #162, #5

## Durable crossing

The rendered real-Switch run built from merged PR #242 durably progresses
beyond StaticR.rel `RelProlog (0x8055531C)`.

The strongest durable snapshot records:

```text
dispatch count        : 24544
post-main dispatch    : 23938
RKSystem::run hits    : 1
StaticR dispatches    : 269
TaskThread::run hits  : 2
AsyncDisplay endRender: 84
GXFlush hits          : 84
RMCP01 FIFO writes    : 1240
FIFO produced work    : YES
GXCopyDisp calls      : 84
present successes     : 84
present failures      : 0
```

The non-zero StaticR dispatch count plus a distinct later DOL/OS blocker proves
that the merged RelProlog native seam is crossed. A seam hit alone is not being
used as proof.

Resource/scheduler invariants remain intact:

- FST: 64,224 bytes / 2,096 entries, structurally valid;
- `/Boot/Strap/eu/English.szs`: 299,969-byte read-pass;
- `/rel/StaticR.rel`: 4,903,876-byte read-pass into `0x805102E0`;
- independent liveness watchdog remains ACTIVE through the final samples;
- default guest fiber/current/running context returns coherently to
  `0x80347498`.

## New exact blocker

```text
kind   = DIRECT
target = 0x801AA4EC
r3     = 0x901187C0
r4     = 0x00000005
r5     = 0x00000000
r6     = 0x9011375C
r7     = 0x90113774
r8     = 0x90113774
stage  = RMCP01_GX_FLUSH
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps `0x801AA4EC` exactly to
`OSDetachThread`.

The live `r3=0x901187C0` is the TaskThread OSThread previously created and
resumed on hardware. Earlier thread diagnostics prove this object exists and
has entered its worker fiber; they do not, however, capture its exact
`state/attributes/join queue/list links` at the instant of this later
OSDetachThread call.

## Pinned semantics

Pinned WiiCompiled performs:

1. disable interrupts;
2. set the OSThread detached attribute bit;
3. read the current thread state;
4. if the state is MORIBUND, unlink the thread from the global thread list,
   clear its guest state to 0, and update host-fiber termination state;
5. wake every waiter on the thread's join queue;
6. restore interrupts.

The state-dependent MORIBUND path makes a blind implementation unsafe without
the exact live thread state. The current Switch guest-fiber seam also does not
yet expose the same termination API used by the pinned desktop runtime.

## Diagnostic-only candidate

The current candidate changes no scheduler behavior. For exactly
`OSDetachThread (0x801AA4EC)`, the durable blocker record additionally captures:

```text
os detach thread
os detach readable
os detach state/attr
os detach suspend/prio
os detach queue
os detach next/prev
os detach join h/t
os detach list n/p
os thread list h/t
os detach fiber known
```

This is sufficient to determine whether the first hardware-observed detach
takes only the live/non-moribund path or requires the full MORIBUND cleanup
contract.

No neighboring OS thread API is pre-ported.

## Next hardware acceptance

Run the private rendered build and inspect `fast-track-dispatch-blocker.txt`
first.

If the target remains `0x801AA4EC`, use the captured OSThread state to
implement only the exact observed pinned OSDetachThread path.

If the blocker moves, the newer durable frontier supersedes this diagnostic
candidate.

Visual Mario Kart Wii pixel correctness remains separately unproven.
