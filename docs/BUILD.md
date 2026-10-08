# Build and hardware diagnostics

Detailed setup, local targets, runtime reports and validation boundaries. See [the project overview](../README.md) for the concise status.

For the private generated game product and pinned renderer dependencies, start
with the [rendered fast-track guide](M3_RMCP01_RENDERED_FAST_TRACK.md). Desktop
diagnostics have a separate [capture/replay guide](DESKTOP_GX_REPLAY.md).

## Important limitations

### Graphics

The normal fast-track GX FIFO bridge remains intentionally a sink, so it stays a reliable **headless control baseline**. The separate rendered fast-track now has hardware-proven real RMCP01 FIFO work **and a successful game-facing `GXCopyDisp → g_surface.Present()`** with `hadWork=1`. Renderer viability and the first GPU present are therefore proven. The remaining graphics question is visual/game-content correctness and the later game/resource path, not whether Aurora/Dawn/NVK can present RMCP01 work on Switch.

### Filesystem / DVD

The project does not fabricate Nintendo game data. The user's own RMCP01 `DATA/sys/fst.bin` is hardware-proven to publish into guest MEM2 at `0x97DC0000`, and the narrow local `DATA/files` DVD bridge is now hardware-proven to service a real boot resource read: `/Boot/Strap/eu/English.szs`, 299,969 bytes. This validates the FST/file mapping on the current boot path; broader DVD semantics remain incomplete and continue to be added only when hardware reaches them.

### IOS / network

The observed boot-time IOS network path is `/dev/net/kd/request`, mode 0.
Hardware has crossed exact KD request sequences for fd 2000 through fd 2003,
including command 2 (Boot probe), command 1 (suspend), command 0x0F
(generated-user-id), and command 3 (resume). Closes for fd 2000 through fd 2003
are hardware-crossed. No generic IOS/network, ioctlv, NCD, IP, SSL, DNS,
socket, or online-play support is claimed.

### Input

Some pinned WPAD/PAD initialization boundaries are mirrored because hardware reached them, but full Joy-Con / Pro Controller / Wii Remote / GameCube input semantics are **not** implemented yet. Input behavior is added only when hardware evidence proves the required boundary and pinned semantics.

## Local fast-track hardware test

After pulling `main`:

```sh
git checkout main
git pull
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-local-fast-track.sh
```

For repeat local rebuilds, the incremental helper also exists:

```sh
MKW_JOBS=4 bash scripts/build-local-fast-track-incremental.sh
```

Copy `WiiCompiled-Switch-local-fast-track.nro` to the Switch and launch it through hbmenu in application/title-override mode with full memory.

Keep that headless NRO as the control baseline and build the separate rendered target:

```sh
MKW_JOBS=4 bash scripts/build-local-rendered-fast-track.sh
```

This produces `WiiCompiled-Switch-local-rendered-fast-track.nro`. It contains locally generated game-derived code and must not be uploaded or committed.

The headless control target intentionally has no renderer. Both targets use durable SD diagnostics rather than a text console:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-thread-events.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

For routine sharing, consolidate the generated diagnostics into one compact
archive:

```sh
python3 scripts/package-fast-track-run.py /path/to/copied/WiiCompiled-Switch
```

Use `--full` when scheduler/thread/liveness history is needed. Runtime still
writes the full durable diagnostics on SD while first-frame bring-up remains
active. `fast-track-heartbeat-history.txt` remains the strongest prolonged
stall diagnostic.

## Public CI boundary

Public CI remains Nintendo-data-free. A real WiiCompiled Mario Kart Wii product is generated locally from a user-owned dump before the Switch build and linked into the NRO. Generated game-derived C++/objects/data and game-containing NRO/ELF artifacts are never committed or uploaded by public CI.

The repository currently validates five Nintendo-data-free CI workflows for fast-track changes:

- `lint`;
- `fast-track-startup`;
- `stateful-translated-sequence`;
- `bootstrap-register-prelude`;
- `build-switch`.

For rendered RMCP01 work, these five checks are **necessary but not sufficient**.
The public `build-switch` workflow also compiles every rendered HLE bridge with
`MKW_LOCAL_RENDERED_FAST_TRACK=1` against pinned WiiCompiled/Aurora headers to
catch rendered-only C++ regressions. The private rendered build remains a
required sixth gate because public CI cannot include the user-owned generated
RMCP01 product or the complete private rendered link graph. Build it through
`scripts/build-local-rendered-fast-track.sh` or the documented equivalent
rendered/Discovery target in a validated prepared tree.

A boundary is only called **hardware-crossed** when attributable runtime
evidence places it on the executed path **and** execution durably progresses
beyond it; a hit counter or first-hit record alone is not a PASS. See
[`docs/FAST_TRACK_VALIDATION_POLICY.md`](../docs/FAST_TRACK_VALIDATION_POLICY.md).

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, decrypted content, or generated translated game output**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See [`LEGAL.md`](../LEGAL.md).

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

Expected public probe output:

```text
WiiCompiled-Switch.nro
```

The public probe contains no translated Mario Kart Wii product. Game-derived translation is a separate local-only build path.

## Current architecture

![WiiCompiled-Switch current architecture](../docs/assets/current-architecture.svg)

<details>
<summary>Text-only architecture summary</summary>

```text
user-owned RMCP01 dump
  -> WiiCompiled static translation (PPC -> generated C++ / RuntimeConfig / data init)
  -> native AArch64 .nro
  -> Switch platform adapter / Wii OS & SDK compatibility HLE
  -> GX/FIFO -> Aurora GX -> Dawn/WebGPU -> Vulkan/NVK -> Switch GPU
  -> libnx/Horizon filesystem + system services
  -> input HLE (partial; controller mapping pending)
  -> audio HLE (partial; native output not hardware-proven)
  -> custom firmware / Nintendo Switch
```

</details>

The diagram reflects the current hardware-proven architecture: translated
AArch64 execution, real resource loading, real GX FIFO work, the
Aurora GX -> Dawn/WebGPU -> Vulkan/NVK graphics chain, and repeated GPU
presents are proven on Switch hardware. Native audio output, complete
controller mapping, a visually correct Mario Kart Wii frame, and full
playability are not yet proven.

For the opt-in first-frame capture NRO, bounded format and private SD output, see [Desktop GX capture/replay](DESKTOP_GX_REPLAY.md).

## Inspect the actual Switch image

Use the optional [surface-image diagnostic](SWITCH_FRAME_DUMP.md) to read the
final NVK surface into first/latest PNGs on SD. It defaults to OFF and can run
alongside first-frame FIFO recording. Its report distinguishes completed-frame
readback from a later unsubmitted partial frame.
