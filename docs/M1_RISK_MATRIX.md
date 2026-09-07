# M1 risk matrix

| Area | Risk | Why | First proof |
|---|---|---|---|
| Guest VM | High | 4 GiB fixed flat address space, aliasing and page-protection/fault semantics are central to hot-path translated memory | libnx VM probe on hardware |
| Graphics | High | Aurora assumes SDL3 + current WebGPU/Dawn/GPU backends; no Horizon backend exists upstream | Deko3D/NXVK feasibility spike |
| HostContext | Medium-High | Existing AArch64 context ABI helps, but stack/ABI/OS plumbing is Darwin/Linux-specific | standalone Switch context-switch test |
| Audio | Medium | Runtime currently depends directly on SDL3 audio | Audren output smoke test |
| Threads/time | Medium | Standard C++ may work but native assumptions/TLS/timing need validation | std runtime probe |
| Filesystem | Medium-Low | SD access is straightforward; `std::filesystem` compatibility still needs validation | filesystem probe |
| Input | Low | libnx HID already works in bootstrap | controller mapping adapter |
| Networking | Deferred | Not required for first offline boot | post-playability milestone |
