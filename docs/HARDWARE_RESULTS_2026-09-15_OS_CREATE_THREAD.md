# Hardware result: VI callback path crossed, OSCreateThread reached

Date: 2026-09-15
Tracking: #117

## Result

A real Switch run built from `main` after PR #137 advanced past the previously implemented VI path and stopped at a new unsupported direct boundary:

```text
kind    : DIRECT
target  : 0x801A9E84
pc      : 0x800060A4
r1      : 0x803990D8
r2      : 0x8038EFA0
r3      : 0x8042A680
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

The progression proves both of the immediately preceding boundaries were crossed on real hardware:

1. `VIWaitForRetrace` (`0x801B99EC`) — the non-fiber Switch retrace/commit bridge returned successfully.
2. `VISetPostRetraceCallback` (`0x801B9138`) — PR #137 registered the guest callback and execution continued.

The new PAL RMCP01 boundary is `OSCreateThread` at `0x801A9E84`.

## Pinned contract

Pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4` models `OSCreateThread` with desktop host-fiber support plus the RVL guest-visible thread/context initialization. The Switch fast-track does not have `GuestFiberManager`, so the host-fiber association is not part of this bridge.

The required guest-visible state includes the embedded `OSContext`, thread state/priority/suspend fields, stack markers and guard, queue/list fields, the scheduler-initialized slow-path state, and global thread-list linkage protected by the OS interrupt-state HLE. The function returns `1` on success and `0` on invalid priority or guest-memory failure.

## Implementation boundary

The Switch bridge keeps creation and scheduling separate. It creates the guest `OSThread` state but does not invent a Horizon host thread/fiber or proactively implement resume/context-switch behavior. If later boot requires `OSResumeThread`, scheduler selection, context loading, or another thread primitive, hardware must identify that boundary first.

## Next hardware acceptance

After the bridge is merged, rebuild the local fast-track NRO from `main` and run it on the real Switch. `0x801A9E84` must no longer appear as the unsupported direct dispatch. The next durable blocker becomes the next implementation step.

A black screen remains expected while GX FIFO writes are deliberately headless.
