# Hardware result: OSCreateThread crossed to OS__InitMessageQueue

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch run from `main` at `e1465f9650f54b3e228a5d8c8686ed1aa2fa7095` crossed the PAL `OSCreateThread` native boundary added in PR #138 and stopped at:

```text
kind    : DIRECT
target  : 0x801A72FC
pc      : 0x800060A4
r1      : 0x803990C8
r2      : 0x8038EFA0
r3      : 0x804294F0
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This hardware result validates the `OSCreateThread` bridge far enough to continue into OS message-queue initialization.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801A72FC` is `OS__InitMessageQueue`.

The pinned native HLE reads:

- `r3`: guest `OSMessageQueue*`;
- `r4`: guest message-array pointer;
- `r5`: message capacity.

For a non-null queue it clears the send and receive `OSThreadQueue` head/tail pairs, writes the message-array pointer and capacity, and resets the circular-buffer `first` and `used` counters. It has no return value and does not fabricate scheduling or wakeups.

Guest layout used by the pin:

```text
+0x00 sendQueue.head
+0x04 sendQueue.tail
+0x08 recvQueue.head
+0x0C recvQueue.tail
+0x10 msgArray
+0x14 msgCount
+0x18 firstIndex
+0x1C usedCount
```

## Switch fast-track contract

The Switch bridge mirrors only that guest-visible initialization. A null or unmapped queue is contained locally, matching the native HLE boundary. No message send/receive behavior, scheduler transition, host queue, or speculative threading behavior is introduced here.

A Nintendo-data-free synthetic probe keeps the exact direct target represented in CI.

## Acceptance

After this bridge merges, rebuild the local fast-track NRO from `main` and run it on real Switch hardware. `0x801A72FC` must no longer be the durable unsupported direct dispatch. The next blocker defines the next implementation step.
