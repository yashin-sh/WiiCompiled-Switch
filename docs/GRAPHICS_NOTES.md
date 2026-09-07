# Graphics backend notes

Aurora upstream currently couples its application layer to SDL3 and exposes GX through D3D12/Vulkan/Metal/WebGPU-oriented code paths.

For Horizon/libnx we will evaluate two routes:

1. **Deko3D native backend** — established low-level Switch homebrew GPU API, likely best control/performance but highest integration work.
2. **Vulkan/NXVK experiment** — potentially reuses more Aurora Vulkan concepts, but must be treated as experimental until Dawn/WebGPU feature compatibility and hardware stability are proven.

Decision rule: prefer the route that minimizes CPU overhead on Tegra X1 while preserving enough of Aurora's GX implementation to avoid a full renderer rewrite.

Do not commit to one backend until the M1 graphics probe has concrete hardware measurements.
