# Strikers / Aurora reference audit — 2026-09-17

This note evaluates `new-coke/strikers` and current `encounter/aurora` as secondary engineering references for WiiCompiled-Switch, with a narrow focus on the two prepared follow-up tracks that matter most now:

- #154 — local-only RMCP01 DVD/FST/resource mapping;
- #4 / #162 — GX/Aurora graphics backend and first-frame work.

The exact pinned WiiCompiled revision remains the primary runtime contract:

`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`

Real Switch hardware remains authoritative for deciding when runtime behavior is merged. The material below is architecture/research input, not permission to pre-port unrelated systems.

## What Strikers is — and is not

`new-coke/strikers` is a native PC port of Super Mario Strikers built from a source decompilation. It is explicitly **not** a static recompilation project.

That means it is not a replacement for WiiCompiled and does not help with PPC translation, HostContext continuation, guest register state, RMCP01 address dispatch, or Wii-specific IOS/HLE semantics.

It is useful because it demonstrates a complete Nintendo SDK game running through Aurora with:

- Aurora GX rendering;
- Vulkan / Metal / D3D12 desktop-class backends through Dawn/WebGPU;
- modern input;
- DVD/FST-backed game-data lookup;
- extracted-folder and disc-image inputs;
- shader/pipeline warm-up behavior;
- a deliberately small compatibility layer for SDK calls Aurora does not provide.

## Licensing boundary

The repository states that the author's original porting code, tools and documentation are offered under CC0 1.0 only to the extent that the author owns those rights. It separately warns that reconstructed game code can remain subject to third-party rights.

Therefore WiiCompiled-Switch should:

- treat reconstructed Strikers game code as reference-only;
- not copy game-specific reconstructed code or assets;
- independently implement RMCP01 behavior from pinned WiiCompiled, hardware evidence and public format specifications;
- prefer upstream Aurora itself for reusable compatibility-layer code because Aurora is MIT-licensed.

## DVD/FST findings for #154

### Strikers pattern

`smstrikers-port/src/platform/dvd.c` separates the host data source from the SDK-facing DVD API.

It can build one logical file index from either:

1. an extracted game-data directory; or
2. a disc image whose FST is walked into the same host-side index.

Each indexed file carries the information needed to service later reads:

- normalized game path;
- host path when backed by an extracted file;
- disc-image byte offset when backed by an image;
- byte length.

The port validates that the selected data actually belongs to the expected game before continuing and reads the disc identity from the user-supplied data rather than hard-coding a full virtual disc.

Aurora upstream has a more general DVD implementation that similarly separates FST entries from underlying read handles and has an asynchronous worker/command model. It can resolve paths and entry numbers while keeping the storage implementation behind an abstract read/seek handle.

### What applies directly to WiiCompiled-Switch

The useful architectural lesson is **not** to replace the pinned WiiCompiled FST contract. It is to split our future implementation into layers:

```text
user-owned RMCP01 source
(extracted DATA root first; optional disc image later)
        ↓
local DiscSource / file-extent index
        ↓
pinned-WiiCompiled FST parser/publication contract
        ↓
guest-visible FST in reserved MEM2
+ low-memory FST address/size
        ↓
DVD path/entry/absolute-offset resolver
        ↓
local host file or image read
```

At the pinned WiiCompiled revision, #154 has already established the required guest-visible contract:

- reserve 2 MiB of MEM2 below the IPC arena;
- parse local `sys/fst.bin` from the user's extracted `DATA` root;
- publish the guest FST into the reserved region;
- write FST address to `0x80000038`;
- write FST size to `0x8000003C`;
- retain file extents so absolute DVD offsets resolve to the corresponding host file;
- convert Wii FST file offsets from four-byte words to bytes.

Strikers therefore strengthens the implementation design without changing these semantics.

### Recommended #154 implementation shape

When #117 hardware actually reaches resource loading, implement the following narrow layers:

1. **Local source discovery**
   - first support the exact extracted layout already expected by pinned WiiCompiled: `DATA/files/` + `DATA/sys/fst.bin`;
   - validate the local disc/header identity as RMCP01 before publishing data;
   - keep image/WBFS support optional and separate from the first hardware-required slice.

2. **Host index**
   - normalized Wii paths;
   - source file path or image extent;
   - byte offset and byte length;
   - deterministic lookup with explicit duplicate/invalid-path handling.

3. **Guest publication**
   - preserve the pinned WiiCompiled MEM2 reservation and low-memory pointers exactly;
   - never synthesize a Nintendo FST in public CI.

4. **Read resolver**
   - path/entry lookup and absolute disc-offset lookup resolve through the same local index;
   - storage failures return diagnostics instead of fake success.

5. **Async timing**
   - keep callback/worker/scheduler timing separate from the data mapping itself until hardware proves ordering matters.

### Nintendo-data-free CI strategy

Public tests can use a fabricated miniature FST and fake files with no Nintendo names or bytes. Cover at least:

- BE FST parsing;
- four-byte-word offset conversion;
- directory traversal;
- path normalization;
- absolute-offset-to-file mapping;
- boundary reads spanning the end of a file;
- missing file;
- malformed FST;
- out-of-range extent;
- wrong fabricated disc identity.

## Graphics findings for #4 / #162

### The key seam already exists in pinned WiiCompiled

The current Switch fast-track deliberately replaces `GX_HLE_FIFO_Write8/16/32/Float/Burst` with a sink.

At the exact pinned WiiCompiled revision, those same helpers normally feed `HleFifoWrite`. The pinned GX runtime already:

- parses GX FIFO command/state traffic;
- tracks vertex descriptors and VAT state;
- expands indexed attributes where required;
- publishes Aurora GX state;
- submits raw/direct draws into Aurora;
- calls Aurora SDK-facing GX functions for draw/state operations.

Therefore **WiiCompiled-Switch should not write a second GX FIFO parser for M3** unless the pinned decoder is proven unusable on Horizon.

The preferred architecture is:

```text
translated RMCP01 GX writes
        ↓
GX_HLE_FIFO_Write*
        ↓
pinned WiiCompiled HleFifoWrite / GX decoder
        ↓
Aurora GX state + draw submission
        ↓
Switch-capable GPU backend
        ↓
libnx/Horizon presentation
```

### What Strikers proves

Strikers is useful evidence for the middle of that pipeline:

- Aurora GX can drive a full Nintendo SDK game, not just synthetic examples;
- the port's explicit missing-GX allowlist is small and currently includes EFB poke/peek-style calls rather than a broad missing renderer surface;
- some game-specific compatibility glue is still necessary (`GXWaitDrawDone`, metrics, fog adjustment, render-mode aliases, PAD sampling callback);
- shader/pipeline compilation is observable enough that Strikers explicitly waits during an early screen for queued pipelines to become ready.

This means first-frame work should include pipeline/cache diagnostics from the start; a black/empty frame can otherwise be confused with a game/runtime failure while pipelines are still compiling.

### What Strikers does not prove

Strikers runs Aurora on desktop/mobile platforms supported by Aurora. It does **not** demonstrate a Horizon/libnx backend.

Current Aurora upstream still describes:

- an SDL3 application/window/input layer;
- GX implemented on WebGPU;
- Dawn as the WebGPU implementation;
- D3D12/Vulkan/Metal backends.

No upstream Deko3D or NXVK-specific backend was found in this audit.

So Strikers validates the GX/Aurora layer, not the final Switch backend.

### Why Dawn/Vulkan/NVK should be the first #4 probe

Issue #4 already contains an external report claiming successful real-Switch rendering through:

`Aurora GX → Dawn/WebGPU → Vulkan (Mesa/NVK) → Switch VI/libnx surface`

That report is not yet independently verified by WiiCompiled-Switch, but it materially reduces the uncertainty of this route.

Combined with the fact that pinned WiiCompiled already targets Aurora, the lowest-risk first experiment is now:

1. preserve pinned `HleFifoWrite` and Aurora GX;
2. create a minimal Horizon presentation path;
3. prove Dawn/WebGPU + Vulkan/Mesa/NVK can create/present a frame;
4. feed a fabricated FIFO sequence through the real WiiCompiled decoder;
5. measure CPU cost, memory use and frame pacing on Tegra X1;
6. only invest in a direct Deko3D Aurora backend if Dawn/NVK is incompatible, unstable or too expensive.

This is now tracked as the narrow child spike #162 under #4.

### Keep Aurora's desktop application layer out of the critical path

Aurora's upstream application layer uses SDL3 for windows, events and input. WiiCompiled-Switch already has native Horizon lifecycle, filesystem and HID services.

The M3 probe should therefore try to retain **Aurora GX + graphics/pipeline code** while replacing or bypassing desktop application/window/input dependencies rather than importing all of SDL3 into the NRO.

The same principle applies to audio and input: use libnx-native services already tracked in M4, not Aurora's SDL device layer, unless a narrow dependency proves unavoidable.

## Decision summary

### #154

Use Strikers/Aurora as implementation-pattern references for source discovery, FST indexing and read abstraction, but preserve pinned WiiCompiled's guest-visible FST/MEM2/low-memory contract exactly.

### #4 / #162

Do not build a new GX parser. The first graphics spike should reuse the exact pinned WiiCompiled FIFO decoder and Aurora GX, with Dawn/Vulkan/Mesa/NVK as the first Horizon backend experiment and Deko3D as fallback.

### Active #117 fast-track

The sustained black-screen runtime has now been hardware-classified as **active** by the 2026-09-18 run: 126,563 translated dispatches with `PostRetraceCallback` sampled at guest retrace value 13,918. This removes the liveness gate for #162. Keep the normal #117 sink as a stable baseline while graphics work proceeds in the isolated M3 probe target; replace it only after that path is hardware-proven.

## Reference priority after this audit

1. pinned WiiCompiled — exact runtime/HLE/GX-decoder semantics;
2. real Switch hardware — implementation gate and behavioral evidence;
3. `doldecomp/mkw` — RMCP01 game/module structure;
4. `encounter/decomp-toolkit` — DOL/REL/disc analysis;
5. `encounter/aurora` — reusable MIT-licensed GX compatibility layer and graphics architecture;
6. `new-coke/strikers` — concrete source-port case study for Aurora GX and DVD/FST integration; architecture/reference only for reconstructed game material;
7. NWiiRecomp — independent architecture reference only under its custom-license restrictions.
