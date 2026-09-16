# Hardware result: OSLoadContext crossed into EGG::Thread::start and OSReceiveMessage

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch fast-track run from `main` after PR #143 validated the non-fiber PAL `OSLoadContext` restore/jump bridge far enough to enter the restored guest thread and stop at:

```text
kind    : DIRECT
target  : 0x801A7424
pc      : 0x8024373C
r1      : 0x8042A628
r2      : 0x8038EFA0
r3      : 0x804294F0
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

Pinned WiiCompiled identifies `0x8024373C` as `EGG::Thread::start`. This proves that `OSLoadContext (0x801A1F58)` restored the selected guest context, dispatched its saved SRR0 through the translated indirect table, and advanced into the new thread. The first unsupported native boundary in that restored thread is PAL `OSReceiveMessage (0x801A7424)`.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801A7424` is `OS__ReceiveMessage_HLE_801a7424`.

The pinned HLE receives:

- `r3`: `OSMessageQueue*`;
- `r4`: optional output message pointer;
- `r5`: flags, with bit 0 selecting `OS_MESSAGE_BLOCK`.

Its guest-visible behavior is:

- return `0` immediately for a null queue;
- disable interrupts once for the operation;
- when the queue contains a message, dequeue the ring-buffer head, advance `first`, decrement `used`, optionally write the message to `*r4`, wake threads waiting to send, restore the previous interrupt state, and return `1`;
- when the queue is empty and the call is non-blocking, restore interrupts and return `0`;
- when the queue is empty and blocking is requested, sleep the current thread on the queue's embedded receive wait queue and retry after it is woken;
- contain guest-memory faults, restore the previous interrupt state, and return `0`.

The ring-buffer fields are the same structure initialized earlier by the already hardware-crossed `OS__InitMessageQueue`: send queue `+0x00`, receive queue `+0x08`, message array `+0x10`, count `+0x14`, first `+0x18`, used `+0x1C`.

## Switch fast-track contract

The Switch bridge mirrors only `OSReceiveMessage` itself. It deliberately leaves the two scheduler dependencies explicit:

- `OSSleepThread (0x801AA9B8)` for an empty blocking receive;
- `OSWakeupThread (0x801AAAA4)` after a successful dequeue frees a queue slot.

Neither scheduler routine is implemented speculatively in this change. The next real hardware run determines which path is actually required first.

A Nintendo-data-free synthetic link anchor keeps the real `OSReceiveMessage` helper reachable through `--gc-sections` without executing guest queue or scheduler semantics against fabricated state.

## Acceptance

Rebuild the local fast-track NRO from merged `main` and run it on real Switch hardware. `0x801A7424` must no longer be the durable unsupported `DIRECT` dispatch. The next durable result should identify the actually exercised scheduler dependency or a later translated/native boundary.
