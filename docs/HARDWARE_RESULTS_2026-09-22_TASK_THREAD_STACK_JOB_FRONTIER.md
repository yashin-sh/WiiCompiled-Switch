# Hardware result — TaskThread receives a stack-frame pointer as a job (2026-09-22)

Tracking: #117, #162, #154

## Result

The rendered real-Switch run preserves the hardware-proven graphics path through
real RMCP01 FIFO work, `GXCopyDisp`, successful surface present and `GXFlush`.
The later durable blocker remains the priority-24 `EGG::TaskThread` worker.

New TaskThread telemetry resolves the tuple:

```text
task thread         : 0x8042BBF0
stack pointer       : 0x8042E458
out message pointer : 0x8042E438
job                 : 0x8042E448
callback            : 0x8042E458
arg                 : 0x80210078
token               : 0x00000000
onDone              : 0x801AA0F0
```

The durable blocker matches `callback=0x8042E458`.

RMCP01's EGG contract sends the address of a heap-allocated `TJob` slot through
the inherited message queue. The observed `job=0x8042E448` instead aliases the
worker's own guest stack. Decoding it as `TJob` produces the stack pointer as
the main callback and `OSExitThread (0x801AA0F0)` as the completion callback.

Therefore this is not a missing translated/native callback. The next hardware
gate is to distinguish whether the producer passed the invalid value to
`OSSendMessage` or a valid send was later read through incorrect queue
array/metadata state.

## Diagnostic candidate

Extend `fast-track-task-thread-last-dispatch.txt` with queue/allocation fields
and add `fast-track-os-message-events.txt` recording each rendered
`OSSendMessage` queue, message, array/count/first/used and caller registers.

No queue, scheduler, HostContext, TaskThread, DVD, GX or resource semantics are
changed.

## Follow-up resolution

The next hardware run resolves the producer-vs-queue question from this
document. `TaskThread::request` sends the valid first job slot
`0x8042E7DC` into queue `0x8042BBFC` / array `0x8042E7A8`. The queue
later shows `first=1`, `used=0`, while the worker reads stack-shaped
`0x8042E448` from its output slot.

The producer-send hypothesis is therefore closed. The active frontier is now
the exact `OSReceiveMessage` phase that clobbers the output slot.
See `HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md`.
