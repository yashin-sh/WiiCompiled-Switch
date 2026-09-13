# M1 risk matrix

| Area | Risk | Why | First proof |
|---|---|---|---|
| Guest VM | High | 4 GiB fixed flat address space, aliasing and page-protection/fault semantics are central to hot-path translated memory | libnx VM probe on hardware |
| Graphics backend | High | Aurora assumes SDL3 + current WebGPU/Dawn/GPU backends; no Horizon backend exists upstream | Deko3D/NXVK feasibility spike |
| GX FIFO safety | Critical before M3 | Safety-critical packet checks still exist behind `CHECK()`, which disappears in Release; malformed/truncated traffic must not become host OOB reads | #109 regression tests in Release-equivalent build |
| GX draw merge compatibility | Critical before M3 | Current merge path does not prove two draws share the same `GXVtxFmt`, so vertex data can be interpreted by the wrong pipeline/layout | #110 mixed-format FIFO regression test |
| GX primitive edge cases | High before M3 | Degenerate `GX_LINESTRIP` counts can underflow instance-count arithmetic | #111 zero/one-vertex tests |
| Indexed XF coverage/diagnostics | High before M3 | Unsupported indexed XF loads may be silently ignored in optimized builds, causing shared state corruption to look like a Switch-backend bug | #112 supported/unsupported XF tests + Release diagnostic |
| HostContext | Medium-High | Existing AArch64 context ABI helps, but stack/ABI/OS plumbing is Darwin/Linux-specific | standalone Switch context-switch test |
| Audio | Medium | Runtime currently depends directly on SDL3 audio | Audren output smoke test |
| Threads/time | Medium | Standard C++ may work but native assumptions/TLS/timing need validation | std runtime probe |
| Filesystem | Medium-Low | SD access is straightforward; `std::filesystem` compatibility still needs validation | filesystem probe |
| Input | Low | libnx HID already works in bootstrap | controller mapping adapter |
| Networking | Deferred | Not required for first offline boot | post-playability milestone |

The GX entries above were added after the 2026-09-13 upstream audit. See `docs/UPSTREAM_GX_AUDIT_2026-09-13.md` for evidence, scope, sequencing, and issue links.
