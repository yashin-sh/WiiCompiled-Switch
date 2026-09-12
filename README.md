# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**Estimated progress toward first rendered frame: ~25%**

```text
█████░░░░░░░░░░░░░ 25%
```

> This percentage is an engineering estimate, not a function-count metric. The pre-graphics runtime has advanced substantially, but a real GX → Switch renderer still does not exist, so the first-frame estimate is intentionally conservative.

| Milestone | Status |
| --- | --- |
| Native Horizon/libnx runtime boots | ✅ Done |
| WiiCompiled PPC → AArch64 translated code executes on Switch | ✅ Done |
| Guest memory/data initialization | ✅ Done |
| HostContext / GuestFlat / translated handoff | ✅ Done |
| Wii SDK early OS/cache/timing/interrupt/bootstrap | 🟡 In progress |
| EXI/SI bootstrap and basic EXI transaction HLE | ✅ Crossed in hardware fast-track |
| Durable blocker / crash diagnostics on SD | ✅ Done |
| Reach Mario Kart Wii `main()` | ⬜ Next major milestone |
| Game/resource initialization | ⬜ Pending |
| GX → Switch graphics backend / first frame | ⬜ Pending |
| Input, audio, filesystem completeness and gameplay | ⬜ Pending |

Current fast-track path:

```text
headless Horizon platform init
  ↓
__start (PAL 0x800060A4)
  ↓
Wii SDK / OS bootstrap
  ↓
cache / timing / interrupts / EXI / SI
  ↓
OSReport
  ↓
OSGetConsoleType
  ↓
OSGetResetCode
  ↓
DCZeroRange
  ↓
IPCCltInit
  ↓
__OSInitSTM
  ↓
NANDInit
  ↓
NANDPrivateOpenAsync        ← latest hardware blocker fixed in main
  ↓
remaining early OS/runtime boundaries
  ↓
__init_user
  ↓
main() (PAL 0x8000B6B0)    ← next major milestone
  ↓
Mario Kart Wii initialization
  ↓
GX / resources / input
  ↓
first rendered frame
```

## Status

**M2 — Fast-track translated startup toward `main()`.** The project executes real WiiCompiled-translated Mario Kart Wii startup code as AArch64 under Horizon/libnx on real Switch hardware.

Validated work includes GuestFlat memory, Wii `Memory::Init`, generated data initialization, translated `__start`, register bootstrap, timebase/SPR/FPSCR helpers, OS timing and interrupt state, exception/interrupt initialization, Wii cache-control HLE, EXI initialization/basic transactions, SI initialization, generic indirect translated dispatch, and a growing set of Wii SDK native/HLE boundaries whose behavior is mirrored from the pinned WiiCompiled runtime.

The hardware-driven guest blocker sequence has now captured and fixed:

- `OSReport` (`0x801A25D0`);
- `OSGetConsoleType` (`0x8019F33C`);
- `OSGetResetCode` (`0x801A8A50`);
- `DCZeroRange` (`0x801A16E4`);
- `IPCCltInit` (`0x80193478`);
- `__OSInitSTM` (`0x801AB848`);
- `NANDInit` (`0x8019E18C`);
- `NANDPrivateOpenAsync` (`0x8019C990`).

`DCZeroRange` exposed an important Switch-runtime contract mismatch: pinned WiiCompiled catches an invalid guest-memory access, while the Switch `Memory::GetPointer` slice returns `nullptr`. A hardware call with `r3 = 0xFFFFFFFF` aligned to `0xFFFFFFE0`, and the old HLE called `memset(nullptr, 0, 0x20)`. The Switch HLE now checks the returned guest pointer before entering libc while preserving valid-range zeroing and the GX/DMA notification seam.

`IPCCltInit` required more than returning success: the Switch HLE calls translated `IPCInit` at `0x80192F7C`, advances the r13-relative IPC buffer-low global by `0x1000` for `iosHeap`, skips Wii-specific interrupt/MMIO setup, and returns success.

`__OSInitSTM` avoids real Wii `/dev/stm/*` IOS devices but preserves the guest-visible reset bookkeeping in the SDA block: initialized flag `1` plus two stable non-zero fake handles. The Switch HLE mirrors those values and guards invalid SDA ranges.

`NANDInit` links the Wii-style guest NAND state to the SD-backed Horizon runtime: it derives the four-character game code from guest memory with PAL `RMCP` fallback, creates the title data directory under `nand_root()`, publishes `/title/00010004/<gamecode>/data` into guest `NANDHomeDir` at `0x80346D20`, writes initialized state `2` at `0x80386848`, and returns `NAND_RESULT_OK` without opening Wii IOS `/dev/fs`.

`NANDPrivateOpenAsync` is the latest hardware-captured `DIRECT` blocker. The Switch bridge now performs a real SD-backed synchronous NAND open below `nand_root()`, normalizes/clamps Wii paths, supports modes 1/2/3, publishes a persistent host fd plus `NANDFileInfo::openFlag = 1`, and invokes the guest completion ABI as `(result, commandBlock)` on a scratch `CpuContext`. Pinned WiiCompiled normally drains queued NAND callbacks later from its alarm/IOS pump; the current fast-track drains the queued callback before the HLE returns. That scheduling difference is explicitly temporary and must be revisited if hardware shows ordering/reentrancy sensitivity.

An earlier hardware run exposed a separate **pre-guest host crash** during `MAIN_PLATFORM_INIT`: no guest context was active, GuestFlat was not initialized, and the fault register state matched an 8 MiB host memory clear in the libnx PrintConsole/NV path. The M2 local fast-track therefore starts **headless** and relies on SD diagnostics until a real GX backend exists. Subsequent hardware runs have confirmed that this headless path reaches `TRANSLATED_EXEC_ENTER` with an active guest context.

The local fast-track build writes durable diagnostics under `sdmc:/switch/WiiCompiled-Switch/` (with a progress-file fallback under `sdmc:/switch/`) so non-crashing black-screen stalls can be distinguished from explicit dispatch blockers and host exceptions. Platform initialization also publishes finer crash stages such as `PLATFORM_SERVICES_INIT`, `PLATFORM_ROMFS_INIT`, and `PLATFORM_READY`.

Important current limitation: the GX FIFO bridge is still a deliberate sink. A black screen is therefore expected even when translated startup is progressing. The project has **not yet proven entry into Mario Kart Wii `main()` and has not rendered a game frame**.

Public CI remains Nintendo-data-free. A real WiiCompiled game product is generated from a user-owned dump **before the Switch build** and linked into the same NRO. Generated game-derived code/data and local game NRO/ELF outputs are never committed or uploaded by CI.

## Local fast-track hardware test

After pulling `main`, build the private local translated product with:

```sh
git checkout main
git pull
MKW_JOBS=8 bash scripts/build-local-fast-track-incremental.sh
```

The incremental helper invalidates its local cache when the Git HEAD changes, verifies that the headless fast-track marker is present in the resulting ELF, and reports the generated NRO SHA-256 so the exact hardware-test artifact can be identified.

Copy the resulting `WiiCompiled-Switch-local-fast-track.nro` to the Switch and launch it through hbmenu in application/title-override mode with full memory.

The local fast-track is intentionally headless: do not expect a libnx text console. Use the SD diagnostic files instead.

Current diagnostics may include:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

`fast-track-main-reached.txt` is written only when the translated dispatcher reaches PAL `main` at `0x8000B6B0`.

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, decrypted content, or generated translated game output**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See `LEGAL.md`.

## Requirements

- A Nintendo Switch capable of running homebrew under Atmosphère
- Homebrew Menu (`hbmenu`)
- devkitPro with `devkitA64` and `libnx`
- GNU Make

Initialize the pinned WiiCompiled source and build the Nintendo-data-free runtime probe:

```sh
git submodule update --init --recursive
make
```

Expected output:

```text
WiiCompiled-Switch.nro
```

Copy it to:

```text
/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro
```

The public probe contains no translated Mario Kart Wii product. Game-derived translation is a separate local-only build path.

## Current architecture

```text
user-owned Wii dump (local build only)
            |
            v
WiiCompiled translator
            |
            v
generated C++ / RuntimeConfig / data init
            |
            +------ linked at build time ------+
                                              |
                                              v
+----------------------------------------------------------+
| Native AArch64 NRO                                      |
| WiiCompiled translated product + runtime                |
| Switch platform adapter                                 |
| - lifecycle / applet                                    |
| - runtime filesystem + diagnostics / NAND backing       |
| - input                                                 |
| - audio (bootstrap/HLE incomplete)                      |
| - graphics (GX FIFO sink; renderer pending)             |
| - context switching / timing / guest memory             |
+----------------------------------------------------------+
            |
            v
        libnx / Horizon
            |
            v
       Atmosphère / Switch
```

## Roadmap and evidence

See:

- `ROADMAP.md`
- `docs/M2_RUNTIME_BOOTSTRAP.md`
- `docs/TRANSLATED_PRODUCT_BOUNDARY.md`
- `docs/HARDWARE_RESULTS_2026-09-12.md`

## Upstream

The long-term aim is to keep Switch-specific changes narrow enough that they can eventually be proposed upstream to WiiCompiled rather than maintaining a permanent fork. The audited upstream revision is recorded in `UPSTREAM.md` and enforced by CI.
