# PAL OSSendMessage — 0x801A735C

Tracking: #117

## Hardware blocker

The first run after #189 reaches a new direct unsupported boundary after the
VI guest-fiber starvation is hardware-fixed:

```text
kind   : DIRECT
target : 0x801A735C
r3     : 0x8042BBFC
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

## Pinned attribution

At WiiCompiled pin
`a135beb201042b20f390c6695ca6b26768820fb4`, this address is
`OS__SendMessage_HLE_801a735c`.

The implementation uses the existing guest `OSMessageQueue` ring-buffer
contract. With interrupts disabled, it appends when the queue has space, wakes
the receive wait queue at offset `0x08`, and returns success. A full
non-blocking send returns 0. A full blocking send sleeps on the send wait queue
at offset `0x00` and retries after wakeup.

## Switch implementation

The Switch bridge reuses the already hardware-proven message-queue layout,
interrupt bridge, `OSSleepThread`, and `OSWakeupThread`. It adds no host-side
queue or scheduler shortcut.

Acceptance is hardware crossing this address to the next exact blocker.
