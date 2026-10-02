# RMCP01 global Discovery Scan

The Discovery Scan reduces the number of real-Switch round trips without
turning unknown Wii behavior into guessed behavior.

It has two complementary layers.

## 1. Static local-product coverage scan

The local translated RMCP01 product already contains every statically emitted
`InvokeDirectCpu<...>` dependency. The scanner compares all of those direct
targets with:

- Switch `KnownNativeCpuCall` coverage;
- generated/local `KnownTranslatedCpuCall` traits;
- optional first-hit evidence from a Discovery NRO trace.

Run it directly:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py
```

JSON:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py --json
```

After a Discovery NRO run:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py \
  --trace /path/to/fast-track-discovery-targets.txt
```

This means we can inspect all statically missing direct boundaries before
hardware reaches them one by one.

## 2. Runtime Discovery NRO

Build:

```bash
MKW_JOBS=4 bash scripts/build-local-rendered-discovery-scan.sh
```

Output:

```text
WiiCompiled-Switch-local-rendered-discovery-scan.nro
```

The Discovery build records the first runtime hit of up to 8192 distinct guest
targets to:

```text
/switch/WiiCompiled-Switch/fast-track-discovery-targets.txt
```

Each record contains the target, dispatch index, guest PC/LR, r1-r8, r13,
current guest fiber and fast-track stage.

The 8192-slot table in `source/fast_track_crash_diagnostics.cpp` is fixed-size and allocation-free. A target is written only on its
first hit, so a 600k-dispatch run does not generate a 600k-line file.
Records are emitted before the callee executes. A first-hit entry proves
arrival, not return. The native/translated path increments the dispatch
counter; an unknown DIRECT frontier records the current counter without a
new increment. VI polling can add callback dispatches before that record.

A later distinct caller/frontier, coherent captured state and verified loop
control flow can establish repeated returns without logging every iteration.
This is the method used to accept ten texture-matrix returns in the
[latest accepted report](HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md).
Scope and limits are defined in the
[validation policy](FAST_TRACK_VALIDATION_POLICY.md).

## Safety rule

Discovery mode does **not** blindly return from unknown calls.

Unknown or stateful boundaries still produce the normal durable blocker and
hard-stop. This is intentional: skipping a scheduler, NAND write, GX state
change, callback or other stateful call would corrupt control flow and make all
later "discoveries" unreliable.

Acceleration comes from:

1. statically exposing all missing direct dependencies at once;
2. recording which covered targets are actually reached and in what order;
3. implementing bounded audited GX setter families with wrapper/Aurora,
   argument and any required guest-mirror contracts;
4. preserving the first trustworthy stateful hard frontier.

## Bundle one artifact for analysis

After copying diagnostics from one run to a new local directory, generate
coverage from that same first-hit trace, then include it in the ZIP:

```bash
python3 scripts/scan-local-rmcp01-dispatch-coverage.py --json \
  --trace /path/to/copied/WiiCompiled-Switch/fast-track-discovery-targets.txt \
  > local-product/rmcp01-dispatch-coverage.json
python3 scripts/package-fast-track-run.py \
  /path/to/copied/WiiCompiled-Switch \
  --full --raw \
  --coverage local-product/rmcp01-dispatch-coverage.json
```

The archive then contains both:

- the runtime first-hit trace and normal hardware diagnostics;
- `rmcp01-dispatch-coverage.json`, which lists every statically emitted direct
  target and highlights coverage gaps.

The Discovery NRO is therefore a scanner, not an emulator fallback.

Record candidate revision, exact NRO size/SHA-256 and transfer outcome alongside
the archive. Retained SD files are not automatically fresh; compare hashes
against the previous run and use the attributable blocker/trace cohort.
See [log bundle guidance](FAST_TRACK_LOG_BUNDLES.md). Static missing-target
counts are coverage gaps, not a count of future hardware blockers.

The [coordinate candidate](GX_TEX_COORD_BATCH_2026-10-03.md) has completed
local validation, a private build and transfer, but has no retrieved new
hardware reports or screen observation. Its anticipated TEV path remains
analysis until those reports establish progression.
