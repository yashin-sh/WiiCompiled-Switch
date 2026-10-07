# Strikers Switch reference update — 2026-10-07

This note supersedes the Switch-specific conclusion in
`docs/STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md`.

The September audit correctly treated `new-coke/strikers` as a useful Aurora/GX and
DVD/FST reference, but at that time concluded that Strikers did **not** demonstrate a
Horizon/libnx backend. That statement is no longer current.

## New evidence

The current Strikers development documentation now contains a complete Nintendo Switch
build path. It explicitly requires an arm64 host and Docker, builds NVK, starts a
`devkitPro` container, fetches Switch dependencies, configures with
`tools/toolchain-switch.cmake`, rebuilds and packages a `strikers.nro`, and documents
launch through hbmenu/title mode or `nxlink`.

Reference:

- https://github.com/new-coke/strikers/wiki/Development#nintendo-switch

The documented sequence includes:

```text
. tools/switch/deps.env
./tools/switch/build-nvk.sh
docker run ... "$DEVKITPRO_IMAGE" ...
./tools/fetch-switch-deps.sh
./tools/build-ffmpeg.sh --switch
CMAKE_TOOLCHAIN_FILE=$PWD/tools/toolchain-switch.cmake ./tools/configure.sh build-switch Release
./tools/switch/rebuild.sh build-switch
./tools/switch/package.sh build-switch strikers-switch
```

and then either copying `build-switch/strikers.nro` to the SD card or launching it with
`nxlink`.

This removes the main uncertainty identified by the September audit: Aurora/Dawn/NVK is
not merely a desktop reference for this family of ports; Strikers now demonstrates the
same broad graphics stack on real Switch homebrew.

## What this validates for WiiCompiled-Switch

This does **not** make Strikers a drop-in implementation for WiiCompiled-Switch.
Strikers is a native source port from a decompilation, while WiiCompiled-Switch executes
statically recompiled RMCP01 PowerPC code through WiiCompiled runtime/HLE semantics.
The difficult parts that remain unique to this project include guest state, translated
control flow, MEM1/MEM2 publication, IOS/HLE boundaries and exact RMCP01 GX traffic.

However, the graphics tail is now independently validated in a much stronger way:

```text
WiiCompiled-Switch
translated RMCP01 code
        ↓
GX_HLE_FIFO_Write* / HleFifoWrite
        ↓
Aurora GX
        ↓
Dawn / WebGPU
        ↓
Vulkan / NVK
        ↓
Horizon / Switch presentation
```

Strikers now provides a working external reference for the lower half of this path.
This strengthens the current project decision to keep Aurora + Dawn + NVK rather than
pivot to a second renderer or a direct Deko3D backend before evidence requires it.

The current WiiCompiled-Switch hardware evidence already proves that real RMCP01 FIFO
work reaches Aurora and that `GXCopyDisp -> Present()` succeeds repeatedly. Therefore
the remaining black-screen investigation should prioritize state/resource correctness
and frame-content observability rather than renderer replacement.

## High-value differential audit

The next comparison should remain narrow and evidence-driven. Do not wholesale-port
Strikers code. Use it as a behavior/reference oracle for the following seams.

### 1. Aurora / Dawn / NVK initialization

Compare:

- adapter/device creation;
- required Dawn/Vulkan toggles and extensions;
- surface creation and swap/present configuration;
- color/depth attachment formats;
- command submission and present ordering;
- resize/viewport initialization;
- synchronization around frame boundaries.

Goal: establish that WiiCompiled-Switch differs only where its architecture requires it.
Any unexplained Switch-specific initialization delta should become an explicit test.

### 2. Shader and pipeline readiness

Strikers exposes shader/pipeline warm-up as an observable runtime concern. A black frame
must therefore be distinguished from a frame whose required pipelines are still queued,
failed, or missing.

WiiCompiled-Switch should record, per rendered frame or bounded diagnostic window:

- requested pipeline count;
- successfully created/ready pipeline count;
- pending pipeline count;
- failed pipeline count and first failure reason;
- draw attempts skipped because a pipeline was unavailable;
- shader-cache hits/misses where available.

The diagnostic must remain optional and must not alter normal frame ordering.

### 3. Texture state and upload semantics

The current RMCP01 frontier is already deep inside real UI/Mii texture-object loading.
Use Strikers/Aurora as a reference for host-side GX texture interpretation, but preserve
WiiCompiled's pinned guest semantics.

For the next bounded hardware run, capture at least:

- GX texture format, logical dimensions and tiled byte range;
- source guest physical address and validated mapped host range;
- texture unit/slot;
- wrap/filter/LOD state;
- whether the resulting Aurora texture was created successfully;
- whether that texture is subsequently bound by a draw before the next present.

The important new question is not only whether `GXLoadTexObj` returns, but whether the
loaded object participates in a draw that contributes to the presented frame.

### 4. Present/frame observability

A successful present counter does not prove that game pixels were generated.
Strikers has a useful deterministic capture mechanism:

- `STRIKERS_CAPTURE=<file.ppm>`;
- configurable capture frame;
- optional exit after capture.

Reference implementation search result:

- `smstrikers-port/src/Game/main.cpp`, where the port requests a binary PPM frame
  capture and performs the readback on the frame's own Aurora command encoder.

For WiiCompiled-Switch, implement the same *concept*, independently:

- opt-in capture of one exact presented frame;
- GPU readback tied to the same command stream/frame being presented;
- Nintendo-data-safe public tests using synthetic content;
- local/private hardware capture allowed for user-owned RMCP01 data;
- capture metadata containing dispatch index, FIFO count, present count and active
  translated function/frontier.

A captured all-black image is substantially stronger evidence than a visually observed
black screen because its exact frame and render path can be correlated with logs.

### 5. Deterministic input / boot-path automation

Strikers also documents:

- `STRIKERS_RECORD_INPUT` / `STRIKERS_REPLAY_INPUT`;
- `STRIKERS_SEED`;
- `STRIKERS_FIXED_DT=1`;
- `STRIKERS_AUTOPRESS=A@200,START@400`.

Its port input layer merges scheduled synthetic button presses with real pad input.

WiiCompiled-Switch should adopt only the architecture idea, not game-specific code:

```text
frame/dispatch trigger
        ↓
optional diagnostic input schedule
        ↓
existing WPAD/PAD bridge
        ↓
normal translated RMCP01 consumer
```

Initial scope should be intentionally small:

- one controller/channel;
- button press/release only;
- frame-number schedule;
- disabled by default;
- log every injected transition;
- no behavioral changes when the feature is disabled.

This would remove a large amount of manual console interaction from repeated black-screen
experiments and make hardware evidence much more reproducible.

## Recommended implementation order

1. **Frame capture first.** It gives a hard artifact for the exact frame that is currently
   only observed as black.
2. **Pipeline readiness counters.** Determine whether real RMCP01 draws are blocked by
   shader/pipeline state.
3. **Texture-to-draw correlation.** Track whether the just-crossed Mii/UI texture loads
   reach an actual bound draw before present.
4. **Deterministic button schedule.** Automate repeated boot/UI navigation once the
   existing PAD/WPAD bridge can safely accept it.
5. Revisit renderer architecture **only** if these diagnostics produce evidence that the
   Aurora/Dawn/NVK path itself is incorrect.

## Explicit non-goals

- Do not replace WiiCompiled's GX decoder with Strikers game code.
- Do not copy reconstructed Strikers game code or Nintendo assets.
- Do not broaden generic IOS/network support from this comparison.
- Do not make synthetic input active in normal builds.
- Do not infer visual correctness from present counters alone.
- Do not pivot to Deko3D solely because the screen is still black.

## Licensing boundary

The original September audit's licensing guidance remains unchanged. Treat reconstructed
Strikers game code as reference-only. Prefer independently implemented diagnostics and
upstream permissively licensed Aurora interfaces where reuse is actually appropriate.
No Nintendo game data belongs in public CI, source, fixtures or captures.

## Decision update

The September statement:

> Strikers validates the GX/Aurora layer, not the final Switch backend.

is now superseded for project planning.

The working assumption as of 2026-10-07 is:

> Strikers independently validates an Aurora/Dawn/NVK Nintendo Switch path strongly
> enough that WiiCompiled-Switch should keep its current renderer architecture and focus
> first on RMCP01 state/resource correctness, pipeline readiness and deterministic frame
> observability.

Real WiiCompiled-Switch hardware evidence remains authoritative for every RMCP01 runtime
change.
