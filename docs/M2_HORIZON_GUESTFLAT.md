# M2 Horizon GuestFlat backend

## Status

Implementation branch: `feat/m2-horizon-guestflat`

This backend turns the hardware-validated VM probes into a reusable Horizon/libnx memory layer shaped after WiiCompiled's `GuestFlat` model. It uses only synthetic test data and does not require Nintendo game content.

## Validated hardware constraints

Real Switch probes established that:

- a contiguous 4 GiB virtual-address window can be found and reserved through libnx;
- `svcCreateSharedMemory` works in the homebrew process;
- one SharedMemory object can be mapped simultaneously at a guest VA and an independent host VA;
- writes are coherent in both directions;
- `svcSetMemoryPermission` cannot re-protect SharedMemory mappings (`0xD401`), so Horizon cannot copy the PC backend's fault/reprotect policy directly.

## Backend layout

The implementation in `source/horizon_guest_flat.cpp` mirrors WiiCompiled's logical backing model:

- `Backing::Owned`: one independent backing for a region;
- `Backing::Mem1`: one shared backing for Wii MEM1 physical/cached/uncached aliases;
- `Backing::Mem2`: one shared backing for Wii MEM2 physical/cached/uncached aliases.

Initialization:

1. classify and size the requested logical backing stores;
2. reserve a VA-only 4 GiB guest window with `virtmemFindAslr` + `virtmemAddReservation`;
3. allocate only the physical SharedMemory needed by requested backing stores;
4. map one permanent host view per backing;
5. map the same backing into every requested Wii alias family inside the guest window;
6. expose `host_pointer(guestAddress)` for runtime/HLE access.

No 4 GiB physical allocation is performed.

## Why complete backing mappings are used

Horizon `svcMapSharedMemory` maps a complete SharedMemory object and has no file-style offset argument. For MEM1/MEM2, the Switch backend therefore maps the complete logical backing at the start of each alias family referenced by the region layout. `host_pointer()` still exposes only explicitly requested Wii ranges.

This is acceptable for the first Horizon integration because protected/special accesses must remain on WiiCompiled's checked path on Switch. It also preserves coherent physical/cached/uncached aliases without creating a kernel handle per Wii page.

## Dynamic guest base

Generic AArch64 WiiCompiled currently assumes the compile-time fixed base `0x1000000000`. On the tested Switch that address is inside Horizon's reserved Alias region, so SharedMemory cannot be mapped there even when `svcQueryMemory` reports it unmapped.

The Switch backend therefore uses a runtime-selected guest base. Upstream integration must replace the generic AArch64 constant on `MKW_PLATFORM_SWITCH` with a runtime base accessor (or a later reserved-register optimization). This is an integration/performance task, not a VM feasibility blocker.

## Checked-access policy

On Switch, MMIO, deferred reads, and executable-write guards must not depend on guest-view page reprotection. The integration should keep those accesses on WiiCompiled's existing checked `Memory::*` policy path.

The initial backend exposes `requires_checked_access_for_special_ranges()` as an explicit contract. A later performance pass can make ordinary RAM accesses use direct guest-base arithmetic while retaining checked routing for special ranges.

## Hardware smoke test

The NRO now runs a synthetic backend smoke test after the VM and HostContext probes. It requests:

- MEM1 physical `0x00000000`, cached `0x80000000`, uncached `0xC0000000`;
- MEM2 physical `0x10000000`, cached `0x90000000`, uncached `0xD0000000`;
- one independent `Owned` region.

The test verifies host-to-guest and guest-to-host coherence across the alias families, validates `host_pointer()`, tears the backend down, and appends results to `sdmc:/switch/WiiCompiled-Switch/vm-probe.txt`.

## Next integration steps

After the hardware smoke test passes:

1. adapt WiiCompiled `guest_flat_memory.h` for `MKW_PLATFORM_SWITCH` and a runtime guest base;
2. move/merge this backend behind the upstream `GuestFlat` API;
3. wire `Memory::Init` region requests into the Horizon backend;
4. force special MMIO/deferred/executable-guard accesses through checked policy on Switch;
5. link the first Nintendo-data-free subset of the WiiCompiled runtime into the NRO;
6. only then proceed to user-supplied local Mario Kart Wii translated output.
