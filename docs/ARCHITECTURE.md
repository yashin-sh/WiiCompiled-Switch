# Switch port architecture

## Principle

Do not rewrite Mario Kart Wii. Reuse WiiCompiled's translated-code/runtime architecture and implement a narrow Horizon/libnx host backend.

## Platform boundary

The port should isolate Switch-specific behavior behind interfaces instead of sprinkling `#ifdef __SWITCH__` across translated game code.

Initial backend areas:

1. **Process/lifecycle** — applet loop, fatal handling, exit.
2. **Memory** — guest virtual address layout, page permissions, fault strategy and alignment.
3. **Threads/context** — host threads, synchronization and coroutine/context switching.
4. **Time** — monotonic clock, sleep/yield, frame scheduling.
5. **Filesystem** — SD-card paths used to emulate the Wii NAND/DVD view.
6. **Input** — Joy-Con/Pro Controller mapping into Wii input abstractions.
7. **Audio** — Horizon audio output backend.
8. **Graphics** — the largest unknown. Aurora/WiiCompiled's current GPU abstraction must be audited against available Switch homebrew APIs and performance constraints.
9. **Networking** — sockets and online compatibility later, after offline play works.

## Performance constraint

A Linux-on-Switch report for WiiCompiled already indicates the Tegra X1 is substantially below full speed in normal races. A Horizon-native port removes Linux/window-system overhead but does **not** automatically solve CPU/runtime cost. Profiling and runtime optimization are first-class milestones, not cleanup work.
