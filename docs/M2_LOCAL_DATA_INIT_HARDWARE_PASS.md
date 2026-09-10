# M2 — Local Mario Kart Wii data-init hardware PASS

Date: 2026-09-10

Issue: #21.

Pinned WiiCompiled revision: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Result

A local-only build produced from user-owned clean PAL RMCP01 inputs was launched on real Nintendo Switch hardware through hbmenu application/title-override mode.

The returned Nintendo-data-free runtime report showed:

```text
critical SDL path      : NONE
lifecycle              : READY
filesystem             : READY
timing                 : READY
libnx HID              : READY
Memory::Init           : READY
HostContext scheduler  : READY
HostContext handoff #1 : READY
HostContext continuation: READY
audio backend          : STUBBED
graphics backend       : STUBBED
translated product     : LINKED
translated product ABI : expected=1 reported=1
translated product id  : local-wiicompiled-product
translated build       : user-owned WiiCompiled generated data initializer
data-init handoff      : ENABLED
data-init provider     : LINKED
data-init handoff ABI  : expected=1 reported=1
data initializer       : AVAILABLE
data sections init     : PASS
stop point             : DATA_SECTIONS_INITIALIZED
```

This proves the real WiiCompiled-generated DOL/REL data-section initializer can populate the Switch runtime's hardware-validated checked guest-memory implementation and return cleanly through the explicit handoff ABI.

No game-derived source, binary blob, DOL, REL, disc image, generated C++, or private NRO is stored in this repository.

## What this does not prove

`DATA_SECTIONS_INITIALIZED` does not execute translated Mario Kart Wii functions. It does not initialize the persistent PPC `CpuContext`, run DOL/REL static constructors, call the game entry point, or enable graphics/audio.

## Next checkpoint

The remaining #21 boundary is build-only:

1. run pinned `translate-recursive 0x800060A4` locally;
2. run pinned `emit-build-shards` locally;
3. consume the generated base function, registration, and dispatch shards from the libnx build;
4. prove they compile/link into a private NRO;
5. still stop at `DATA_SECTIONS_INITIALIZED` without invoking any translated function.

Only after that link-only boundary passes should persistent PPC context and the first translated execution handoff be introduced.
