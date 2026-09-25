# Hardware result — KD command 2 crossed / IOS_Close frontier (2026-09-25)

Tracking: #117, #154, #162

## Hardware evidence

The rendered real-Switch run built from merged PR #235 preserves the exact KD
open result:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2000
```

The first KD command-2 Boot probe is now hardware-crossed:

```text
status=cmd2-boot-probe-pass
fd=2000
cmd=2
in=0x80356F20/0x00000020
out=0x80356F40/0x00000020
```

A status hit is not the crossing proof by itself. The proof is the later,
distinct durable blocker:

```text
kind   = DIRECT
target = 0x80193AD8
r3     = 0x000007D0
stage  = HOST_CONTEXT_SWITCH_RETURNED
```

Execution therefore progresses beyond `IOS_Ioctl (0x80194290)` and reaches a
new native boundary.

## Preserved invariants

The same run keeps the established resource, scheduler and graphics path:

- FST published at `0x97DC0000`, 64,224 bytes / 2,096 entries;
- `/Boot/Strap/eu/English.szs` read-pass at 299,969 bytes;
- SZS decode-pass: 299,969 bytes consumed / 2,627,200 bytes produced;
- `GXInitTexObj` remains init-pass at 832x456, format 4;
- `TaskThread::run hits = 1`;
- `VIWaitForRetrace hits = 6`, `OSReceiveMessage hits = 4`,
  `OSSleepThread hits = 4`, `SelectThread hits = 13`;
- 29 RMCP01 FIFO writes with produced work;
- `GXCopyDisp calls = 1`;
- one successful surface present and zero failures;
- the independent watchdog remains ACTIVE before the terminal blocker.

This preserves the prior GPU-present proof but still does not claim visually
correct Mario Kart Wii pixels.

## Pinned mapping

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x80193AD8` exactly to:

```cpp
NAND_IOS_Close_HLE(uint32_t fd)
```

The live ABI has only one argument, and hardware supplies:

```text
fd = r3 = 2000
```

That is the same network-device handle returned by the already crossed
`/dev/net/kd/request` open.

Pinned `NAND_IOS_Close_HLE` checks whether the fd is a network handle. For
fd 2000 it routes to `Network_HLE_Close(fd)`. That function removes the
device from the network handle map and returns `0`.

## Minimal candidate

The Switch bridge therefore handles only this observed close:

1. require fd 2000;
2. require the locally tracked first KD handle to still be open;
3. retire that one local handle;
4. emit rendered-only `fast-track-ios-close-kd-request.txt`;
5. return IOS result `0` in r3.

Any other fd remains a fresh unsupported boundary. No generic IOS_Close table,
KD command 1/3, repeated command 2, ioctlv, NCD, IP, SSL, DNS, sockets or other
network behavior is added.

## Next hardware acceptance

After merge with all five public CI gates green, the next private real-Switch
run must:

1. preserve the KD open and command-2 proofs plus the established
   FST/DVD/SZS/scheduler/GX invariants;
2. record `fast-track-ios-close-kd-request.txt` as close-pass;
3. durably progress beyond `0x80193AD8` or prove a later milestone;
4. use the next exact blocker, native exception or genuine liveness stall as
   the sole following work item.

A close status file alone is not sufficient to call the boundary crossed.
