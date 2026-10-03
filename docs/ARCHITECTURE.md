# Switch port architecture

## Implemented boundary

The port reuses WiiCompiled's static PowerPC translation and implements a
Horizon/libnx host layer. It does not rewrite the game or run Dolphin at runtime.
The runtime/Aurora source is pinned to
`a135beb201042b20f390c6695ca6b26768820fb4`; the rendered backend uses the
documented Dawn and Mesa pins. Game-derived translation and assets remain local.

```mermaid
flowchart TD
    dump[User-owned local game dump] --> generated[Local translated C++ and data]
    generated --> dispatch[Switch ABI and dispatch]
    dispatch --> memory[Checked guest memory]
    dispatch --> fibers[Cooperative guest fibers]
    dispatch --> hle[Audited native HLE bridges]
    hle --> io[Local DVD, NAND and platform services]
    hle --> fifo[GX state and FIFO decoder]
    fifo --> aurora[Aurora]
    aurora --> dawn[Dawn WebGPU]
    dawn --> nvk[Vulkan / NVK / Horizon surface]
    dispatch --> reports[Durable diagnostics and unknown-call blockers]
```

The public Makefile builds Nintendo-data-free probes and synthetic execution
seams. The separate `local-rendered-fast-track/CMakeLists.txt` links the local
translated product with Aurora/Dawn/NVK. The headless fast-track deliberately
retains its FIFO sink as a control baseline. Discovery adds first-hit tracing;
it still aborts at unsupported calls or unproven bridge arguments.

## Memory and ABI

`source/horizon_guest_flat.cpp` reserves a runtime-selected 4 GiB address-space
token and allocates backing from hbloader's existing heap. MEM1/MEM2 aliases
share their backing sections. The reserved guest window is not directly mapped:
`GuestFlat::RequiresCheckedAccess()` stays true, and translated accesses use
the checked Switch Memory path. Deferred protection/executable-range hooks are
explicit no-ops under that policy; fault counters do not measure a desktop
page-fault implementation.

`source/memory_switch_slice.cpp` resolves a complete requested range before
forming a host pointer. GetPointer/Contains reject unavailable, zero-length or
overflowing ranges. Read/Write operations preserve guest big-endian order and
throw AccessViolation for unavailable backing. This is a correctness-first
implementation; linear region lookup remains a potential profiling target.

`include/abi_bridge.h` prefers known native overrides, then generated direct
translated traits. It preserves the pinned nonvolatile floating-point contract.
Indirect calls use the generated immutable dispatch table plus explicitly
ported native overrides. Unknown direct/indirect targets produce durable
diagnostics and terminate; adding a generic success fallback would invalidate
the bring-up evidence.

## Scheduling and platform services

Guest scheduling runs cooperatively on one host thread. Each guest fiber owns
a native stack and saved CpuContext; the handwritten AArch64 context switch
preserves platform state and callee-saved registers. Guest current/running
identities are checked when a suspended caller resumes. VI service points poll
due retraces while respecting the ported interrupt state, so callbacks can add
dispatches and change the diagnostic stage before the original call proceeds.

DVD/FST/SZS, NAND, IOS/KD, input and audio bridges implement bounded paths
attributed to pinned WiiCompiled and real hardware evidence. Their availability
is not a claim of complete Wii filesystem, controller, audio or online support.
Many guards still describe exact observed PAL RMCP01 states. Other game
profiles and arbitrary scheduler/resource states are outside this validation.

## Graphics and evidence

The rendered fast-track connects guest GX FIFO operations to the pinned
decoder and Aurora, then presents through Dawn/NVK. Native GX setters preserve
Aurora state/cache semantics; guest bookkeeping is mirrored only where the
pinned wrapper requires it. Frame activation and presentation are explicit
contracts, not operations to add to every new setter by analogy.

Hardware has established real FIFO work and repeated successful presents.
The latest accepted TEV run preserves ten type-0 texture-matrix loads and
all eight exact Gen2/disabled-Scale/disabled-Bias triples, then returns from
six scalar setters across stages 0..15 on caller default tuples. The new
frontier is KColor ID 0, guest pointer `0x80398FCC`, dispatch 605056.
Enabled Scale/Bias and alternate TEV inputs retain host contracts only.
The user reported black output and an error at exit. KColor pointer handling
remains a hard stop. SIZE_MAX rejection, Present(false), teardown exceptions
and shutdown recovery were not exercised on hardware.
The separate [TEV color/table candidate](GX_TEV_COLOR_BATCH_2026-10-03.md)
now passes all five GitHub workflows and its exact private build (code
`1333b0e2`, NRO `a56be881...`). KColor and adjacent Color/SwapModeTable are
implemented with bounded memory/enum guards; fresh console return remains pending.
See
[`HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md`](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md),
[`HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md`](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
[`HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md`](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md),
[`FAST_TRACK_VALIDATION_POLICY.md`](FAST_TRACK_VALIDATION_POLICY.md) and
[`GX_TEX_COORD_BATCH_2026-10-03.md`](GX_TEX_COORD_BATCH_2026-10-03.md).

Presentation counters alone do not establish recognizable game pixels.
Snapshots can precede a newly ported bridge, native Aurora writes can bypass
the instrumented guest FIFO counter, and Discovery records first occurrences
rather than every iteration. Acceptance therefore combines fresh reports,
caller/control flow, coherent context and a later durable milestone.

## Method and remaining engineering work

The chosen bring-up method is appropriate for exposing exact runtime gaps:
attribute the observed frontier, preserve its pinned contract, execute narrow
synthetic tests, compile/link the real rendered candidate, then retest on Switch.
Audited bounded GX families may be batched to reduce rebuilds and console round
trips. Stateful memory, scheduling, callback, DVD and resource changes still
need their own hardware-defined boundaries. Static forecasts prepare the next
review; they cannot establish a hardware return.

The architecture is a working bring-up layer, not a completed compatibility
runtime. The following work remains:

- generalize exact-state guards only after their semantics and legal domains
  have evidence and tests; avoid an ever-growing list of descriptor exceptions;
- broaden executable scheduler/ABI/resource contracts: synthetic NRO symbol
  retention and syntax checks do not execute the console code;
- audit the private rendered link's broad `--allow-multiple-definition` policy
  and replace it with explicit ownership where possible;
- move repeated build preparation toward stable, content-preserving inputs,
  and validate a pinned CI toolchain image before claiming reproducibility;
- profile translated dispatch, checked memory, diagnostics and GPU work once
  a representative game scene runs. Native Horizon execution does not itself
  establish playable performance.

These limits identify the evidence needed before making broader compatibility
or performance claims.


## Latest console result — TEV colors crossed (2026-10-03)

The [fresh color/table hardware result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
supersedes the earlier pending color/table status. All twelve executed calls
returned through the coherent later caller; AlphaCompare `0x80172088`,
(7,0,0,7,0), is the new DIRECT hard stop. Black output and a crash persist.
Actual RGBA bytes and recognizable game pixels remain unproven. The elapsed
time includes an unexplained watchdog sampling gap, so it is not a performance
measurement. Prior dated results above retain their original scope.
