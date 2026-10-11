# Bounded TEV indirect configuration — 2026-10-11

The [corrected KSEL trial](GX_TEV_KSEL_2026-10-11.md) advances beyond the former
color-selector frontier and pauses at DIRECT `GXSetTevIndirect`
(`0x801717AC`), dispatch 659,106, after 49.175 seconds of translated execution.
The fresh durable report retains `RMCP01_GX_SET_TEV_K_ALPHA_SEL` as the preceding
stage. This is evidence from candidate `4e5087d`, whose exact-source private
rendered build, all eight published exact-HEAD checks and complete SD readback
pass. Its 73,711,672-byte NRO has SHA-256 identity `22dac60ee6a2eda3…`.

## Observed arguments and evidence limits

The report records `r3` through `r8` as zero. Those registers cover the TEV stage,
indirect stage, format, bias, matrix and S wrap. It does **not** record `r9`, `r10`
or the two stack arguments. The T wrap, add-previous flag, original-LOD flag and
alpha selector therefore remain unknown for this hardware call. The correction
must read and validate them at runtime; the six observed zeroes cannot establish
a complete all-zero tuple or justify skipping the native setter.

Only 63 bytes of live log were collected because subscription began after the
guest had already paused. The durable report establishes the frontier; a complete
live sequence, automatic NRO restart, new frame-rate results and menu pixels are
not established. Binary products, full hashes, raw CPU reports and derived caller
analysis remain private.

## Checked ten-argument bridge

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` defines a ten-u32
wrapper for this setter. The bridge follows its ABI: the first eight arguments
are in `r3`–`r10`; the ninth and tenth are big-endian u32 spills at `sp+8` and
`sp+12`. It requires initialized guest memory and one complete readable eight-byte
spill before accessing either word. A 64-bit end calculation rejects address
wrap, and the actual Memory containment and pointer checks reject unmapped,
short or split regions.

The pinned wrapper clamps TEV stage to 15 and indirect stage to 3. The bridge
preserves those clamps and converts both full-u32 boolean arguments by testing
nonzero; a value such as `0x100` remains true. Enum validation precedes native BP
output:

| Argument | Admitted domain |
| --- | --- |
| Indirect format | 0–3 |
| Bias | 0–7 |
| Matrix | 0–3, 5–7, 9–11 |
| S and T wraps, independently | 0–6 |
| Alpha selector | 0–3 |

Matrix holes, enum sentinels, oversized values and unreadable spills stop with a
specific diagnostic before the setter is called. The bridge preserves guest CPU
state, emits its own stage and routes this missing native extension through the
existing extension registry. It uses the pinned GX writer and Aurora decoder
without modifying a guest GXData mirror. A complete valid all-zero tuple still
performs the native write. In this pinned renderer, the original-LOD flag is
decoded and stored but not consumed by `shader.cpp`; forwarding it does not
establish complete indirect-shader behavior.

## Validation and remaining gates

Local production bridge/Memory/registry contracts pass 9,598 admitted cases
and 61 diagnosed refusals in each headless/rendered mode under ASan, fatal UBSan
and LeakSanitizer. The tests use the pinned
ten-argument wrapper, actual native writer and BP decoder as oracles. They cover
the enum domains, separate wrap axes, wrapper clamps, full-width booleans,
big-endian spills, relocated and boundary stack addresses, preservation of CPU
and unrelated GX state, actual packets and decoded state. Negative cases include
matrix holes, high-bit inputs, address overflow, uninitialized memory, truncated
spills and adjacent regions that do not contain the complete spill.

Independent private SDK compile/partial-link and registry checks pass with the
actual direct-call traits and extension-to-native-provider path. The
public indirect rerun and full rendered/input/Discovery/capture SDK gate pass.
The combined candidate's exact-source private rendered build and complete
exact-HEAD CI rollup remain pending. KSEL's successful `4e5087d` gates
do not validate this new source. The next NRO also includes the separate
[coordinated hbloader route](DEBUG_SERVER.md), gated on loader capability and a
proven owner shutdown path. Program restart is unsupported even with its reserved
configuration flag: application mode cannot prove a forwarder mapping, and
hbmenu title override can restart its retail host game. Automatic relaunch still
requires attributable hardware validation.

No corrected indirect NRO has yet been deployed or observed on Switch. Its
complete SD readback, verified backup/cleanup and attributable indirect return
are separate pending gates. No automatic restart or later game pixels are
claimed by the bridge contracts.
