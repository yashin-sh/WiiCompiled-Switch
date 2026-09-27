# PAL OSSetPeriodicAlarm frontier — `0x801A08E0`

The 2026-09-27 rendered real-Switch run hardware-crosses the fourth exact
`GXInitTexObjLOD` descriptor on `obj=0x9018E480` and reaches:

```text
target = 0x801A08E0
alarm  = 0x802D58B8
start  = 0x00000000EA6EDB94
period = 0x0000000000000000
r13    = 0x8038CC00
```

Pinned WiiCompiled attributes the target to `OSSetPeriodicAlarm`. The
matching RVL decomp confirms the guest-visible contract: repeat/begin field
writes plus sorted insertion into the SDA alarm queue.

The Switch candidate implements only that exact function-level RVL contract.
The host decrementer remains absent because pinned WiiCompiled stubs PPCMtdec,
and alarm pumping/callback dispatch remains outside this patch until real
hardware proves it is needed.
