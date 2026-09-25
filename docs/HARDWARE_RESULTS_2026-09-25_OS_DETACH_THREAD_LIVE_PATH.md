# Hardware result — OSDetachThread live path captured (2026-09-25)

Tracking: #117, #155, #162

## Latest durable run

The run built from merged diagnostic PR #243 reaches the same exact blocker
and captures the live OSThread state needed to choose the pinned branch.

```text
DIRECT 0x801AA4EC
r3 = 0x901187C0

os detach readable    : YES
os detach state/attr  : 4 / 0x0001
os detach suspend/prio: 0 / 20
os detach queue       : 0x90113730
os detach next/prev   : 0x00000000 / 0x00000000
os detach join h/t    : 0x00000000 / 0x00000000
os detach list n/p    : 0x00000000 / 0x90112660
os thread list h/t    : 0x80347498 / 0x901187C0
os detach fiber known : YES
```

The strongest snapshot reaches 25,636 translated dispatches / 25,030
post-main dispatches, 269 StaticR dispatches, 1,240 RMCP01 FIFO writes and
84 successful presents with zero failures.

## Exact pinned branch

Pinned WiiCompiled OSDetachThread:

1. disables interrupts;
2. sets the detached attribute bit;
3. reads the thread state;
4. performs list/fiber cleanup only if state == MORIBUND;
5. wakes the join queue;
6. restores interrupts.

The observed thread is state 4 (WAITING), not MORIBUND. Attribute bit 0 is
already set and the join queue is empty. Therefore the hardware-proven path
does not enter the delist/fiber-termination branch.

## Candidate

The candidate accepts only:

- thread pointer 0x901187C0;
- state 4;
- attributes 0x0001;
- empty join queue.

It mirrors the pinned interrupt scope, writes attr|1, invokes the existing
OSWakeupThread HLE for the empty join queue, and restores interrupts.

Any thread/state/join variation aborts as a fresh OSDetachThread frontier.
No neighboring OS thread API and no MORIBUND cleanup are pre-ported.
