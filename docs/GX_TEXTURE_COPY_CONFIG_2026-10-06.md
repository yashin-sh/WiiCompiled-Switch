# Bounded texture-copy configuration — 2026-10-06

The [fresh Fog run](HARDWARE_RESULTS_2026-10-06_MII_FOG_COPY_CLAMP_FRONTIER.md)
crosses the Mii Fog call, then reaches GXSetCopyClamp `0x8016F618`, value 3.
This candidate implements that observed setter and pre-ports the next two
configuration setters from the checked caller, under the authorized bounded
GX setter policy. All three observed tuples now return in the [verified console run](HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md); broader argument families retain host proof.

| Setter | Target | Scope / forecast |
| --- | --- | --- |
| CopyClamp | `0x8016F618` | Full raw word 0..3; observed 3 |
| TexCopySrc | `0x8016F478` | Full r3..r6 narrowed to u16; observed `(0,0,128,128)` |
| TexCopyDst | `0x8016F4DC` | u16 dimensions, full format 5 only; observed `(128,128,5,0)` |

## Pinned semantics

Clamp calls native Aurora first. Native state gets its two low bits, with no
FIFO writes. The WiiCompiled wrapper then reads GXData through `0x803886C8`
and replaces only the low two bits of words `GXData+0x23C` and `GXData+0x24C`.
The bridge keeps its best-effort exception handling: zero/missing GXData leaves
native state updated; a successful first write remains committed if the second
access fails. U32 address additions wrap, and complete scalar accesses use the
existing Switch Memory implementation. Upper bits and other guest bytes stay intact.

GXFBClamp is an unfixed enum with range 0..3. The full raw word is checked before
casting, memory or native work. Other words remain unproven; masking/narrowing
first would admit unsupported inputs. The observed combined top/bottom value 3
is within the enum range even though it is not a separately named enumerator.

Src forwards the four low-u16 coordinates/dimensions to Aurora, which records
the rectangle and sets `texCopySrcRenderSpace=false`, without FIFO writes.
It then updates the same four fields of pinned `TexCopyState`.
Dst forwards u16 dimensions, RGB5A3 format 5 and whole-word nonzero-to-bool mipmap
to Aurora, which stores format/dimensions/half-scale. The HLE shadow retains the
complete raw mipmap word, exactly as WiiCompiled does. Other formats refuse
before native/state work. These setters allocate/copy no textures and dereference
no guest buffers.

The bridge defines the canonical `g_texCopyState` symbol using the actual pinned
header type, because the rendered slice currently excludes `gx_utils.cpp`.
Provider ownership must remain unique. This prepares configuration consumed by
future copy execution; it does not implement GXCopyTex or guest/GPU readback.

All three calls preserve complete CPU bytes and publish separate diagnostic
stages. Null contexts have no effects. Headless mode refuses all three before
native or guest-memory access; synthetic startup only retains their link probes.

## Validation

The production-trait/bridge tests execute the actual Switch scalar-memory range
checker with guarded, shared guest backing. They verify native-before-memory/
HLE-shadow ordering, exact big-endian low-bit mirrors, unaligned/full/short/null/
missing/wrapped addresses, partial writes, complete CPU preservation, full u16
domains with high-word inputs, whole-word bool conversion and raw shadow retention.
Diagnosed SIGABRT refusals require the right stage/target/reason and no native,
memory or shadow effects, including after the report.

The three pinned Aurora setter bodies execute verbatim in a separate fixture,
with only state storage replaced. It checks all four clamps, full u16 source/
destination domains, both mipmap states and untouched sentinel fields.
Targeted validation passes **393,253 valid rendered calls and 12 diagnosed
refusals**, plus **12 headless refusals** with no native/memory effects.
The independent verbatim native setter fixture passes **524,292 state cases**.
ASan, fatal UBSan and LeakSanitizer remain active. All nineteen local host
suites pass, along with both AArch64 modes, full synthetic retention and lint.
Seven compiled mutants are rejected, including native-after-memory ordering
checked through actual scalar Read32/Write32 entry points. All five GitHub
workflows / six jobs pass on integrated code `fec006a`; the actual CI log
includes the new rendered/headless contracts and native state fixture.

The private Rendered Discovery build passes on that same code with pinned
Dawn/Mesa/WiiCompiled dependencies and preserved upstream patch bytes/mtimes.
It retains 56 required strong functions plus canonical `g_texCopyState`.
The scoped provider audit finds one owner for each of 30 symbols across
232 host inputs, 19 Rust archives and seven named libraries. These checks
establish link ownership, not executed copy calls or recognizable pixels.

The NRO is **73,498,680 bytes**, SHA-256
`45d7b6eaec53a3d8df1cdf96832fd76a0d2a6c237fca8e92291e2f790f775e29`.
USB/MTP copies it to
`sdmc:/switch/WiiCompiled-Switch-gx-texture-copy-config-rendered-discovery.nro`
at **2026-10-06 05:50:54 UTC**, with complete byte-for-byte and hash readback.
The earlier direct nxlink attempt ended at 05:51:17 UTC with a connection
failure (exit 1), before transfer. The verified SD file was subsequently
reported launched manually, with black output followed by an error; its
precise launch time is unavailable and no reports from that attempt have been
retrieved. These observations are separate from the later network launch.

[PR #316](https://github.com/yashin-sh/WiiCompiled-Switch/pull/316) merged as
`666e559c9140bb63a1694be2e15d09834a19af68` at **06:05:01 UTC**, after final
documentation-head checks.

## Later direct launch and verified result — 2026-10-06

The Switch replied to UDP netloader discovery at **16:47:21 UTC**. Direct
nxlink started at **16:47:41 UTC** and exited **0 at 16:47:54 UTC**, sending
the exact validated NRO: 26,745,240 compressed bytes / 2,247 blocks (36.39%).
Launch revision `7ef008f8f3747babecf14f96236ff6e765742242` differs from validated
code `fec006a` only in Markdown. Candidate source hashes, dependency pins and
the existing upstream patch were checked before transfer. No TCP preflight
was used.

Fresh [verified reports](HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md)
at 16:54:08 UTC establish return from Clamp(3), Src(0,0,128,128) and
Dst(128,128,5,0), before GXCopyTex `0x8016FD74`, destination `0x9210A720`,
clear 1, dispatch 633774 / 126.018 seconds. All 36 reports / 630,085 bytes
have independently verified manifest, baseline and archive bytes. Acceptance
is limited to these tuples; raw mirrors and GPU state are not captured.
This later run's visual observation, actual texture-copy execution and
recognizable game pixels remain pending. Private NROs, game products and raw archives remain
excluded from the public repository.
