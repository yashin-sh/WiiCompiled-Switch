# GXLoadTexMtxImm bridge — 2026-10-02

## Observed boundary and pinned contract

The accepted eight-map IA8 candidate (`f5d9fdd`, NRO SHA-256
`3cfba800770622a32e843e4869c50d00c1213e861cd24c8c1f6424ab607d0e16`)
reached missing direct target `0x80173234`, after dispatch 607523, with
`r3=0x802581C8`, `r4=30`, `r5=0`, `lr=0x80240F94`. The reports live locally
under `.deps/network-tests/gx-load-ia8-all-maps/runs/20261002T164340Z/raw`.
The user observed a black screen and an exit/crash after at least one minute.
The last active watchdog sample was 106445 ms, dispatch 607350; the later
blocker explicitly records `abort after durable blocker record`. This explains
the observed termination of that run without implying the black screen has a
single cause. No native exception report was retrieved.

At WiiCompiled pin `a135beb201042b20f390c6695ca6b26768820fb4`,
`runtime/src/hle/gx/gx_transform.cpp`, `GX__LoadTexMtxImm_80173234` reads
12 big-endian float32 coefficients for type 0 (`GX_MTX3x4`) and otherwise
8, into a zero-initialized 12-float array. It forwards the array, id and type to
Aurora `GXLoadTexMtxImm`. Aurora validates regular ids 30..60 or post ids
64..125; post ids require type 0. It emits 8 coefficients for type 1 and
otherwise 12. Neither wrapper nor Aurora begins/presents a frame here.

## Implementation

`source/gx_load_tex_mtx_imm_hle_bridge.cpp` implements that matrix decode with
real Switch `Memory::Read32`, retaining coefficient bits and row order.
`KnownNativeCpuCall<0x80173234>` forwards the actual PPC arguments. The
rendered branch calls real Aurora GX; the headless branch still validates and
decodes guest backing. Neither changes guest registers or guest matrix bytes.

Uninitialized, null, unmapped, truncated or wrapping guest ranges record
`GX_LOAD_TEX_MTX_IMM_INVALID_MATRIX` at the pointer in r3 and abort before GX.
The bridge does not invent coefficient ranges, substitute a matrix, normalize
types/ids or soften Aurora's existing validation.

The synthetic link probe retains the real direct-call trait and bridge without
executing fabricated game state. The host contract uses mapped synthetic guest
memory, independent big-endian byte encoding, ASan/UBSan, both render modes,
12/8 coefficients and padding, signed zero and unusual float bits, legal enum types, ids, CPU and
memory preservation, unaligned/range-end cases, and diagnosed SIGABRT failures.
CI executes that contract and checks the retained AArch64 symbols. Rendered
contracts cover type 0/1; other nonzero types are decoded only in headless
contracts. Like the pin, the bridge casts the native argument to GXTexMtxType;
values outside that enum do not have a portable native contract.

## Termination diagnostics

`source/fast_track_crash_diagnostics.cpp` records a host tick at the first
translated dispatch. Blocker reports now include `elapsed_ms` and the dispatch
count; native exception reports include the same elapsed-time measure. Zero
means no first dispatch was recorded yet. This is program execution time from
the first dispatch, excluding nxlink transfer time. Termination behavior is
unchanged; the diagnostic watchdog does not terminate the program.

## Validation and hardware acceptance

Source candidate `02010aaaa974581f49ed2dddfd02ebf0e2bc0e96` passes all
10 lint checks, 28 local workflow steps, and the rendered AArch64 syntax gate.
The five GX host contracts pass in both modes under ASan/UBSan. The new
matrix contract exercises 200 successful headless calls, 110 rendered calls
with legal types 0/1, and 32 diagnosed memory refusals per mode.
GitHub Actions itself was not run.

The local Rendered Discovery build completed with exit 0 after 164 build
steps, 2026-10-02 17:09:43–17:36:06 UTC. The final ELF contains strong
`mkw_switch_hle_gx_load_tex_mtx_imm` and `GXLoadTexMtxImm` symbols.
The NRO is `WiiCompiled-Switch-local-rendered-discovery-scan.nro`,
73,293,880 bytes, SHA-256
`e666413d2c76a2ef9a5a278a4feb4a1e94ae01632464b643a62f885905bb656e`.
Local validation/build metadata is retained under
`.deps/network-tests/gx-load-tex-mtx-imm`; no game fixture or NRO is committed.
The existing integration patch is byte-identical (SHA-256
`92984dd129257e15e006fb7df1d891b6a7799f88620ad560c7a9733d4c19d0f7`).

Hardware acceptance is now recorded in
`HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md`.
The fresh later scale frontier at dispatch 605350 follows the first matrix
at 605340, with coherent caller/state and an exact +10 delta. The executed
loop establishes ten type-0 returns (ids 30,33,...57), then Gen2 coord 0.
This is control-flow evidence, not ten individually logged matrix hits.
The new DIRECT blocker is GXSetTexCoordScaleManually (`0x80171180`),
99,513 ms after the first dispatch. No recognizable game image is established
by these reports; the screen observation for this launch is pending.

## Initial launch

Nxlink transferred the exact candidate to `192.168.1.194` with exit 0,
2026-10-02 17:39:00–17:39:32 UTC (19:39 Europe/Paris). This records the
completed transfer/launch request, not a matrix return or visible image.
USB/MTP retrieval preserved 28 reports (526,625 bytes), with 12 changed
against the previous IA8 run. The user screen observation is pending.
