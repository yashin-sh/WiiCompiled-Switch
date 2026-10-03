# Bounded TEV color and swap-table batch — 2026-10-03

## Observed trigger and bounded scope

The [accepted scalar run](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md)
reaches KColor `0x80171ED4`, ID 0, guest pointer `0x80398FCC`, dispatch
605056, elapsed_ms 98265, LR `0x80240F98`, r1 `0x80398FB8`, stage SwapMode.
The user reported black output and an error at exit. KColor has not returned.
The four RGBA bytes are not present in the reports and are not assumed here.

This separate bounded candidate implements KColor and pre-ports the adjacent
Color and SwapModeTable setters in the same checked caller. The pinned wrapper
and actual Aurora implementation were audited for all three. Neighbor inclusion
reduces build/console round trips; it does not hardware-accept their calls.
ColorS10, konstant selectors and later pixel/other setters remain outside this lot.

WiiCompiled/Aurora pin:
`a135beb201042b20f390c6695ca6b26768820fb4`. Attribution is to
`runtime/src/hle/gx/gx_tev.cpp`, bundled Aurora `lib/dolphin/gx/GXTev.cpp`,
`GXEnum.h`, and the local generated caller `func_80241380`.

## Contracts and native effects

| Entry | Raw registers | Bounded contract |
| --- | --- | --- |
| KColor `0x80171ED4` | r3 ID, r4 guest RGBA pointer | ID 0..3 before lookup; complete readable four-byte range |
| Color `0x80171E10` | r3 ID, r4 guest RGBA pointer | ID 0..3 before lookup; complete readable four-byte range |
| SwapModeTable `0x8017200C` | r3 ID, r4..r7 red/green/blue/alpha selectors | all five unsigned words 0..3 before enum conversion |

Each void CpuContext bridge preserves the whole CPU and guest memory. A null
CPU has no stage, lookup or native effects. Non-null calls publish a distinct
`RMCP01_GX_SET_TEV_*` stage before validation. Invalid raw arguments diagnose
`GX_SET_TEV_*_UNPROVEN_ARGS`, the exact function target and unchanged CPU,
then abort before memory/native work. Unreadable colors diagnose the separate
`GX_SET_TEV_*_UNREADABLE_COLOR` reason and abort before native work.

The pinned pointer wrappers guard ID < 4 before `Memory::GetPointer(r4,4)`
and copy p[0],p[1],p[2],p[3] into GXColor r,g,b,a by value. The candidate uses
the actual Switch Memory seam for the entire range and explicitly rejects
nullptr; the pin assumes that pointer is readable. Address zero is accepted
when mapped to physical MEM1. Unaligned readable pointers are valid. No host
word reinterpretation, guest-base addition, alignment restriction, endian swap
or guessed color constant is introduced. Rendered mode copies the four bytes
and forwards once to the matching Aurora setter. Headless mode retains ID and
range guards without native effects. No guest GXData mirror, dirty flag,
frame activation, frame-work marker or presentation is added.

Aurora KColor emits the RA/BG BP pair at 0xE0+2*ID / 0xE1+2*ID with the
KColor bank bit 23 set. Color emits its TEV register pair without that bank bit;
the pinned software implementation omits redundant SDK timing writes. Both
set host bpSent=1. SwapModeTable updates the low four bits of the two shared
tevKsel words at indices 2*ID and 2*ID+1, preserving neighboring konstant
selector fields, then emits both BP words and sets bpSent=1. Native Aurora
owns those state and FIFO effects; the bridges do not reconstruct them.
The pin guards SwapModeTable ID but leaves channel enums unchecked. The new
guard deliberately bounds all channels to their legal SDK domain 0..3.

## Executable host contract and CI wiring

[`test-gx-tev-color-batch.sh`](../scripts/test-gx-tev-color-batch.sh) executes
the actual bridges, traits and Switch Memory::GetPointer implementation in
rendered=0/1 with ASan/UBSan fatal and LSan active. The allocation-only Horizon
seam supplies shared backing ending at protected pages; forwarding-only Aurora
sinks observe calls rather than model GPU behavior. GNU linker instrumentation
on the Linux 64-bit host counts actual bridge lookups while delegating to the
real Memory method. It proves invalid IDs reject before any lookup, and valid
pointer calls resolve exactly four bytes once.

Each mode passes 9,504 valid calls and 132 diagnosed SIGABRT refusals:

- all IDs, every byte value in each RGBA position with asymmetric neighbors,
  zero/all-255 colors, unaligned pointers, exact region ends, mapped address
  zero and the last valid four bytes below 4 GiB;
- all 1,024 legal swap-table tuples;
- raw invalid IDs/enums including 256 and UINT32_MAX, initialized/uninitialized
  and reset memory, unmapped pointers, partial tails and 32-bit wrapping ranges;
- CPU and all mapped guest bytes compared after successful calls and after
  child termination using shared CPU/backing, including mutations after reports;
- exactly one correct refusal report before real abort, no native calls on
  refusals, and null CPUs with no effects.

A native attempt in a refusal child exits with a distinct status to avoid
mistaking a native assertion for the expected abort. The tests do not establish
native BP decoding, full guest alias allocation, GPU pixels or console execution.
Existing memory contracts retain alias/range implementation coverage. Three
local mutation checks reject lookup-before-ID, reversed RGBA and CPU mutation
after reporting; only temporary source copies were changed.

`build-switch` runs this tenth host contract. `fast-track-startup` retains the
three direct traits/bridges with a link-only synthetic probe and checks all
four strong text symbols. The rendered syntax gate automatically discovers
all three bridges. Makefile/CMake source globs include them without a separate
hand-maintained source list.

## Forecast and acceptance requirements

The checked caller loops KColor IDs 0..3 using r1+20, then Color IDs 0..3
using r1+16. With the accepted stack, these are `0x80398FCC` and `0x80398FC8`.
Both values originate from the same four guest small-data bytes. Only the
first KColor pointer is captured; Color's pointer and all actual RGBA bytes
remain forecasts. The subsequent SwapModeTable tuples are
(0,0,1,2,3), (1,0,0,0,3), (2,1,1,1,3), (3,2,2,2,3).

A later caller `0x80241530`, LR `0x80240F9C`, with restored r1 `0x80398FD8`
could establish return of those twelve calls by checked control flow and
coherent fresh state. Callback-free, first mapped KColor count D would reach
that caller at D+12; callbacks may add dispatches. Its existing BlendMode is
followed by the forecast missing GXSetAlphaCompare `0x80172088`, tuple
(7,0,0,7,0), on the nested r1 `0x80398FC8`. It remains outside this lot.
Neither the exact dispatch delta nor a future image is assumed.

- [x] Actual rendered/headless host contracts with sanitizers.
- [x] AArch64 rendered syntax gate, all current bridges and Discovery diagnostics.
- [x] Exact candidate lint, synthetic link and all five GitHub workflows.
- [x] Private Rendered Discovery build, unchanged pins/patch and native provider check.
- [ ] Successful exact-NRO transfer and fresh attributable console reports.
- [ ] Later coherent progression establishes each executed member's return.

Integrated candidate `1333b0e2c5b6695a2d8512ee1080041792308718` passed
all five GitHub workflows and both build jobs, including all ten host contracts.
The remote logs independently confirm the new contract counts in both modes.
The private Rendered Discovery build completed with exit 0 at
2026-10-03 12:49:28 UTC (14:49:28 Europe/Paris), after 27 minutes 44 seconds,
with Rendered/Discovery ON, three jobs, network disabled and immutable image
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`.

NRO: 73,343,032 bytes, SHA-256
`a56be88113ff7c2cc20808111cf7d6c0e947b0c8955b2252737b974b28a9e0ad`.
The ELF has all 25 checked strong text symbols, including the three new
bridges and three actual Aurora setters. All tracked source hashes stayed
unchanged during compilation; WiiCompiled/Dawn/Mesa pins and the existing
WiiCompiled patch bytes/modification times were preserved. Previous coordinate,
audit and scalar TEV NROs remain intact. The NRO, full logs and validation
metadata remain local under `.deps/network-tests/gx-tev-color-batch/`.

A fresh post-build provider scan checked 222 actual host link inputs, nineteen
Rust archives and seven named libraries inside the same immutable image.
Each new object defines its bridge and references the expected native symbol;
none defines a replacement GX native name. Each native name has exactly one
expected provider in Aurora `libm3_aurora_gx.a:GXTev.o`. The current graph differs
from the previous accepted scalar graph only by these three object inputs.
This covers those three names, not every symbol under the private link's broad
allow-multiple-definition option, nor BP decoding/pixels. Detailed source,
object, graph and artifact hashes remain local.

The latest accepted frontier remains KColor arrival. This new batch is a
candidate; no pointer-setter return or recognizable game image is accepted yet.
