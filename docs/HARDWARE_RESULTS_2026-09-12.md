# Hardware results — 2026-09-12

This document records the real Nintendo Switch evidence for the M2 translated-startup fast-track on 2026-09-12.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Scope

The goal of these runs was not to render graphics. It was to execute the real locally generated Mario Kart Wii translated startup path as far as possible, stop on the first attributable blocker, mirror the pinned WiiCompiled semantics, rebuild, and repeat.

The local build contains user-owned game-derived translated output and is therefore never uploaded to public CI or committed to the repository.

## Confirmed runtime state

Real hardware confirms that the local NRO can:

- initialize the Horizon/libnx host runtime;
- run the M2 local path without the libnx PrintConsole/NV framebuffer dependency;
- initialize Wii guest memory / GuestFlat;
- initialize generated data sections;
- enter WiiCompiled-translated PAL `__start` at `0x800060A4`;
- maintain an active guest context through `TRANSLATED_EXEC_ENTER`;
- execute through multiple Wii SDK/native runtime boundaries;
- execute a mixed native-HLE → translated call path (`IPCCltInit` → `IPCInit`);
- preserve required guest SDA bookkeeping through host HLE (`__OSInitSTM`);
- initialize NAND/ISFS host+guest state through `NANDInit` and advance to the first asynchronous NAND open boundary;
- emit durable unsupported-dispatch and host-exception diagnostics to SD.

The screen remains black because the current GX FIFO bridge is deliberately a sink. No rendered Mario Kart Wii frame has been proven.

PAL `main()` at `0x8000B6B0` has **not** yet been proven reached.

## Hardware blocker sequence

### 1. `OSReport` — `0x801A25D0`

Observed as an unsupported `DIRECT` dispatch during `TRANSLATED_EXEC_ENTER`.

Pinned WiiCompiled behavior: host-side report formatting/logging only, with no guest-visible CPU or memory mutation.

Switch fix: sink the host logging side effect while preserving guest state.

Result after fix: hardware advanced beyond this boundary.

### 2. `OSGetConsoleType` — `0x8019F33C`

Observed as the next unsupported `DIRECT` boundary.

Pinned WiiCompiled behavior:

- read guest physical MEM2 size at `0x80003118`;
- return `0x00000012` for retail 64 MiB MEM2;
- otherwise return `0x10000012` for the NDEV/expanded-memory path.

Switch fix: reproduce the same guest-memory read and return value in `r3`.

Result after fix: hardware advanced beyond this boundary.

### 3. `OSGetResetCode` — `0x801A8A50`

Captured blocker:

```text
kind                  : DIRECT
target                : 0x801a8a50
guest pc              : 0x800060a4
r1                    : 0x80399158
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled behavior: avoid the Wii reset MMIO register and always return `0` (`Cold Boot`).

Switch fix: publish `r3 = 0` through the native/HLE trait.

PR: #76

Result after fix: hardware advanced beyond this boundary.

### 4. `DCZeroRange` — `0x801A16E4`

The first hardware observation was an unsupported `DIRECT` dispatch at `0x801A16E4` during `TRANSLATED_EXEC_ENTER`.

Pinned WiiCompiled behavior:

- align the guest start address down to a 32-byte cache line;
- round the covered length up to whole cache lines;
- zero the resulting guest range;
- notify the GX guest-RAM DMA-write seam;
- treat an invalid guest range as best-effort failure rather than fatal host behavior.

The first Switch HLE mirrored the valid-range zeroing but assumed invalid guest memory would throw like upstream WiiCompiled.

A later hardware run exposed the contract mismatch with this signature:

```text
fast-track stage      : TRANSLATED_EXEC_ENTER
fault address (FAR)   : 0x0000000000000000
x0                    : 0x0000000000000000
x2                    : 0x0000000000000020
x5                    : 0x00000000ffffffe0
guest context active  : YES
guest context source  : TLS
guest pc              : 0x800060a4
guest r3              : 0xffffffff
FAR in guest window   : NO
```

Interpretation:

1. guest `r3 = 0xFFFFFFFF` aligned down to `0xFFFFFFE0`;
2. the normalized size became one 32-byte cache line (`0x20`);
3. the Switch `Memory::GetPointer` slice returned `nullptr` for the invalid guest range;
4. the old HLE entered `memset(nullptr, 0, 0x20)`;
5. Horizon raised the AArch64 Data Abort at FAR `0x0`.

Switch fix: explicitly test the returned guest pointer before `memset`. Valid-range zeroing and the GX/DMA notification remain unchanged; invalid ranges are skipped.

PR: #82

CI result before merge: 5/5 workflows green, including a Nintendo-data-free synthetic case reproducing `address = 0xFFFFFFFF`, `length = 1`.

Result after fix: hardware advanced beyond `DCZeroRange` and produced a new explicit blocker instead of crashing.

### 5. `IPCCltInit` — `0x80193478`

Captured blocker:

```text
kind                  : DIRECT
target                : 0x80193478
guest pc              : 0x800060a4
r1                    : 0x80399168
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled maps `0x80193478` to `IPCCltInit`.

Its HLE is not a simple success stub. It:

1. calls translated `IPCInit` at `0x80192F7C` so the IPC buffer globals are initialized;
2. reads the r13-relative IPC buffer-low global;
3. advances that pointer by `0x1000` for the IOS heap;
4. skips the Wii-specific interrupt handler/MMIO portion;
5. returns success.

Switch fix: reproduce the same mixed native-HLE → translated call sequence and `0x1000` guest-memory adjustment.

PR: #83

CI result before merge: 5/5 workflows green, including the native→translated dispatch/link seam.

Result after fix: hardware advanced beyond `IPCCltInit` and produced the next explicit blocker at `__OSInitSTM`.

### 6. `__OSInitSTM` — `0x801AB848`

Captured blocker:

```text
kind                  : DIRECT
target                : 0x801ab848
guest pc              : 0x800060a4
r1                    : 0x80399168
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled maps `0x801AB848` to `__OSInitSTM_HLE_801ab848`.

The host HLE does not open real Wii `/dev/stm/*` IOS devices. It instead publishes the guest-visible state that later reset logic expects through the SDA block relative to `r13`:

- `r13 - 0x62CC` = STM initialized flag `1`;
- `r13 - 0x62C8` = non-zero immediate handle `0x00535401`;
- `r13 - 0x62C4` = non-zero event-hook handle `0x00535402`;
- return value = success (`r3 = 1`).

Power/Reset callback pointers are intentionally left unset because the Switch fast-track never fires the Wii STM hardware interrupt that would invoke them.

Switch fix: mirror the three guest-memory writes and success value, but first validate all three SDA addresses with the Switch memory slice. A zero or invalid SDA returns failure (`r3 = 0`) instead of becoming a host memory fault.

PR: #85

CI result before merge: 5/5 workflows green, including Nintendo-data-free synthetic fast-track coverage of the `0x801AB848` native dispatch seam.

Merge commit: `a97e3b5e4bac0f0e72e3d832ab0b4d8f47a0b485`.

Result after fix: hardware advanced beyond `__OSInitSTM` and produced the next explicit blocker at `NANDInit`.

### 7. `NANDInit` — `0x8019E18C`

Captured blocker:

```text
kind                  : DIRECT
target                : 0x8019e18c
guest pc              : 0x800060a4
r1                    : 0x80399158
r2                    : 0x8038efa0
r3                    : 0x10000012
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled maps `0x8019E18C` to `NANDInit_HLE`.

The HLE performs both host-side storage bootstrap and guest-visible NAND initialization:

1. initialize host-side ISFS/NAND support;
2. derive the current four-character game code from guest memory at `0x80000000`, falling back to PAL `RMCP` when the value is unavailable/invalid;
3. create the title data directory corresponding to `/title/00010004/<gamecode>/data`;
4. write that Wii-style path into guest `NANDHomeDir` at `0x80346D20`;
5. write initialized state `2` at `0x80386848`;
6. return `NAND_RESULT_OK`.

Switch fix: map the host-side title directory below the existing SD-backed Horizon `nand_root()`, preserve the same Wii-style guest path and initialization state, and skip the real Wii IOS `/dev/fs` device. Host directory creation is best-effort; guest fixed-address writes are range-checked.

PR: #87

CI result before merge: 5/5 workflows green, including Nintendo-data-free fast-track coverage for `InvokeDirectCpu<0x8019E18C>` and the full Switch build.

Merge commit: `56e778ce7a5e8ac678763fd00bc6922f904884ea`.

Result after fix: hardware advanced beyond `NANDInit` and captured `NANDPrivateOpenAsync` at `0x8019C990`.

### 8. `NANDPrivateOpenAsync` — `0x8019C990`

Captured blocker:

```text
kind                  : DIRECT
target                : 0x8019c990
guest pc              : 0x800060a4
r1                    : 0x80399138
r2                    : 0x8038efa0
r3                    : 0x80252df8
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled maps this address to `NANDPrivateOpenAsync_HLE`. This boundary is not a success-only stub. It performs synchronous `NANDOpen` semantics first, queues the completion callback with `(result, commandBlock)`, and returns the same NAND result. Upstream drains pending NAND callbacks later from its alarm/IOS servicing path.

Switch fix: add a persistent SD-backed NAND open runtime below the existing `nand_root()`. Wii guest paths are normalized, backslashes are converted, relative paths resolve below the title data directory, and `.`/`..` traversal is clamped at the NAND root. Modes 1/2/3 map to host read/read-write opens. Successful opens allocate a persistent fd beginning at 100, write the fd into guest `NANDFileInfo`, and set `openFlag = 1` at offset `0x8A`. The boundary preserves the relevant result family (`0`, `-8`, `-12`, `-64`).

The completion callback receives `r3 = result` and `r4 = commandBlock` on a scratch `CpuContext`, preventing callback register mutations from corrupting the interrupted translated caller. The current fast-track queues then drains the callback before the HLE returns; unlike pinned WiiCompiled, it does not yet delay delivery to the later alarm/IOS pump. This is an explicit scheduling approximation to be validated on hardware, not a claim of timing equivalence.

PR: #93

The first PR head exposed only integration issues in CI: `clang-format` formatting and direct inclusion of WiiCompiled `abi_bridge.h` before the devkitA64 GCC compatibility shims. Both were fixed. Fresh head `c03a446d3f3bfde3b107cd3754b8aa9563a8d2ea` passed all 5 workflows, including `fast-track-startup` and the full Nintendo-data-free Switch build.

Merge commit: `5c5cc83f85837396a1ac8db8e80ada16d71ccd97`.

Current status: `NANDPrivateOpenAsync` is the latest hardware-captured blocker fixed in `main`. A post-#93 hardware run is required to prove that the current callback scheduling advances boot, expose any ordering/reentrancy issue, identify the next boundary, or prove PAL `main()`.

## Diagnostic-path issue discovered during hardware testing

One non-crashing black-screen run produced no `.txt` files even though the NRO itself was valid. The fast-track diagnostic path assumed `sdmc:/switch/WiiCompiled-Switch/` already existed and silently ignored file-open failure.

PR #75 changed the local fast-track to create the diagnostic directory and retain a fallback progress file under `sdmc:/switch/`. Later runs successfully produced durable blocker records.

## Pre-guest host crash during platform initialization

A run before the headless change crashed during `MAIN_PLATFORM_INIT` with no guest context, no GuestFlat mapping, and a host clear of exactly `0x800000` bytes. The signature matched the libnx PrintConsole/NV transfer-memory path rather than translated PPC execution.

The local fast-track was therefore changed to start **headless**:

- no `consoleInit()` in `MKW_LOCAL_FAST_TRACK` builds;
- no fast-track `printf`/`consoleUpdate` dependency;
- public/default Nintendo-data-free builds keep the PrintConsole path;
- actionable progress/blocker/exception state is written to SD.

Later hardware evidence validated this change directly: the process reached `TRANSLATED_EXEC_ENTER` with an active TLS guest context and nonzero GuestFlat base.

## Local build provenance / verification issue

After the headless change, a valid ELF/NRO was initially rejected by the final local helper check. The old check used:

```sh
strings "$ELF" | grep -Fq 'PLATFORM_CONSOLE_SKIPPED_FAST_TRACK'
```

under `set -o pipefail`. `grep -q` can close the pipe after the first match, causing `strings` to receive SIGPIPE and making the pipeline nonzero despite a valid match.

The helper now performs a direct binary-safe grep on the ELF. It also tracks source HEAD, invalidates stale incremental objects when the commit changes, and prints the NRO SHA-256 so the exact hardware-test artifact can be identified.

## Current diagnostic files

The local fast-track may create:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

`fast-track-main-reached.txt` is the explicit hardware proof marker for PAL `main` at `0x8000B6B0`.

## Current interpretation

The 2026-09-12 evidence proves real translated startup progress well beyond the original runtime bootstrap and metadata-only product boundary.

Specifically, hardware has now proven that:

- the headless local platform path reaches real guest execution;
- PAL translated `__start` continues across multiple pinned WiiCompiled native/HLE boundaries;
- host-vs-guest faults can be distinguished through durable exception context;
- invalid guest-memory behavior at HLE seams must match the Switch memory slice contract as well as upstream intent;
- native HLE may legitimately call translated guest code and must preserve that dependency;
- hardware-facing host HLE may skip Wii I/O while still publishing mandatory guest SDA state;
- storage HLE requires both host filesystem effects and guest path/state publication;
- `NANDInit` now advances far enough to reach the first concrete async file-open boundary.

It does **not** prove:

- that the new `NANDPrivateOpenAsync` scheduling approximation has advanced on hardware yet;
- entry into PAL `main()`;
- game/resource initialization completion;
- a working GX renderer;
- a rendered frame;
- playable input/audio/gameplay.

The next real-hardware run should be classified as one of:

1. a new `fast-track-dispatch-blocker.txt` — current callback delivery worked well enough to continue; map/fix the next pinned runtime boundary;
2. a `fast-track-exception.txt` — use stage + FAR/registers + guest context to attribute the failure, including possible callback ordering/reentrancy;
3. a heartbeat with no blocker — investigate a non-crashing loop/stall and whether delayed NAND completion is required;
4. `fast-track-main-reached.txt` — declare the PAL `main()` milestone reached and move to the first post-`main` blocker.

## Build command used for the local fast-track

```sh
git checkout main
git pull
MKW_JOBS=8 bash scripts/build-local-fast-track-incremental.sh
```

No game-derived NRO/ELF, generated C++, DOL/REL, disc image, keys, firmware or copyrighted game assets belong in this repository or public CI.
