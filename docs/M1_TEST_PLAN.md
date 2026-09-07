# M1 hardware test plan

These probes intentionally contain no Nintendo game data.

## Probe A — virtual memory

Goal: determine whether Horizon/libnx can support WiiCompiled's flat 4 GiB guest address model.

Measure and log:

- address-space reservation success/failure
- returned base address
- mapping alignment/page granularity
- dual-alias feasibility
- permission transition behavior
- allocation/mapping latency

Pass condition: enough functionality exists to implement a fast guest-memory strategy without relying on undefined behavior.

## Probe B — cooperative AArch64 context

Goal: validate a Switch-native HostContext backend independently of WiiCompiled.

Test:

1. create scheduler context
2. allocate worker stack
3. switch scheduler -> worker -> scheduler repeatedly
4. verify continuation state
5. verify callee-saved GPR/SIMD state
6. stress at least 100k context switches

Pass condition: deterministic state preservation with no stack corruption.

## Probe C — graphics feasibility

Compare minimal triangles/present loops for candidate backends before porting Aurora GX:

- Deko3D baseline
- Vulkan/NXVK only if the environment is available and stable enough

Record CPU frame cost, GPU frame cost where measurable, memory use, and presentation stability.

## Probe D — filesystem/std runtime

Validate:

- `std::filesystem`
- C++ threads/mutex/condition variables
- monotonic clocks
- SD file create/read/write/rename

These results decide how much standard C++ WiiCompiled code can be reused unchanged.
