# Fast-track log bundles

The rendered fast-track intentionally writes durable SD-card diagnostics so a
crash or unsupported dispatch still leaves evidence.

That does **not** mean every generated `.txt` file must be sent for every
run.

## Current strategy

Keep runtime generation unchanged while first-frame work is still active.
Several traces that look redundant during a normal GX run become valuable when
thread scheduling takes a different path.

For routine sharing, use:

```bash
python3 scripts/package-fast-track-run.py \
  /path/to/sd/switch/WiiCompiled-Switch
```

It creates:

```text
fast-track-run-compact.zip
  fast-track-report.txt
  manifest.txt
```

The report concatenates the selected diagnostics with explicit filename
headers, so analysis still retains file boundaries while the user handles one
small artifact.

New/unknown `.txt` diagnostics are included automatically.

## Compact exclusions

Only these historical high-volume traces are omitted from the default compact
report:

```text
fast-track-os-sleep-events.txt
fast-track-os-receive-message-frontier.txt
fast-track-thread-events.txt
fast-track-post-main-trace.txt
fast-track-post-video-trace.txt
fast-track-os-message-events.txt
```

They are not deleted from the SD card.

The durable blocker, heartbeat/watchdog, renderer status, GX/IOS status,
resource status, last-dispatch records and future new status files remain in
the compact report.

## Full mode

When the run shows a scheduler, thread, receive/sleep or unexplained liveness
problem:

```bash
python3 scripts/package-fast-track-run.py \
  --full \
  /path/to/sd/switch/WiiCompiled-Switch
```

To also keep every selected original file inside the ZIP:

```bash
python3 scripts/package-fast-track-run.py \
  --full \
  --raw \
  /path/to/sd/switch/WiiCompiled-Switch
```

## Size example

One real hardware bundle from 2026-09-26 contained:

```text
26 text files
167359 bytes total
```

The six verbose historical traces accounted for:

```text
142677 bytes
```

The compact selection retained:

```text
20 logical diagnostics
24682 bytes of source text
```

So the issue is mostly clutter, not storage pressure.

## Why runtime generation is not reduced yet

A single append-only log looks cleaner, but it is less robust if the process
crashes during a write and can create more contention on the SD path.

The current snapshot-style files also let one subsystem update its durable
state without rewriting an ever-growing global log.

Once a visible Mario Kart frame is hardware-proven and scheduler behavior is
less volatile, the project can safely introduce a compile-time compact runtime
profile that disables selected historical traces. Until then, the bundler
reduces user-facing clutter without reducing diagnostic coverage on hardware.
