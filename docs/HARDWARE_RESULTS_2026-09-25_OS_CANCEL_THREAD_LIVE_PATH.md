# Hardware result — exact OSCancelThread path captured (2026-09-25)

Tracking: #117, #155, #162

## Latest durable run

The run built from merged diagnostic PR #245 reaches the same exact
`OSCancelThread (0x801AA1D4)` blocker and captures the complete first
TaskThread cancellation state.

```text
thread                  : 0x901187C0
state / attr            : 4 / 0x0001
suspend / priority      : 0 / 20
wait queue              : 0x90113730
wait next / prev        : 0 / 0
wait queue head / tail  : 0x901187C0 / 0x901187C0
run queue               : NO
join head / tail        : 0 / 0
mutex                   : 0
owned mutex head / tail : 0 / 0
global list next / prev : 0 / 0x90112660
global list head / tail : 0x80347498 / 0x901187C0
resched / pending       : 1 / 0x02000000
fiber known             : YES
fiber current           : 0x80347498
OS current / running    : 0x80347498 / 0x80347498
```

The strongest durable snapshot reaches 27,052 translated dispatches / 26,446
post-main dispatches, 269 StaticR dispatches, 1,240 RMCP01 FIFO writes and 84
successful presents with zero failures.

## Exact pinned branch

Pinned WiiCompiled OSCancelThread sees state WAITING and therefore:

1. disables interrupts;
2. removes the thread from its wait queue;
3. clears its guest OSContext;
4. sees attr bit 0 set (detached) and removes it from the global thread list;
5. writes final guest state 0;
6. terminates the associated host fiber;
7. runs __OSUnlockAllMutex;
8. wakes joiners;
9. because resched != 0, calls SelectThread(0);
10. restores interrupts.

The observed owned-mutex list and join queue are both empty, so steps 7 and 8
have no guest payload beyond their normal empty-queue bookkeeping.

The target fiber is known but is not the current fiber/HostContext. Therefore
it can be destroyed immediately without deleting the active stack.

## Exact Switch candidate

The candidate accepts only the full hardware-observed state above.

It:

- removes the singleton wait-queue node;
- calls the existing OSClearContext HLE;
- removes the detached tail node from the global thread list;
- writes state 0;
- destroys only the proven non-current guest HostContext;
- treats the exact empty owned-mutex list as the pinned no-op case;
- calls the existing OSWakeupThread HLE for the exact empty join queue;
- calls the existing SelectThread(0) seam while resched remains non-zero;
- restores the outer interrupt state.

Any variation in thread, queue/list shape, scheduler flags, fiber ownership,
mutex/join state, OS current/running context, or pending mask remains a fresh
hardware-defined blocker.

No neighboring OS thread API is pre-ported.
