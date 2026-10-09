# Sampled SD diagnostics and KD post-resume stop — 2026-10-09

The console run of PR #340 code `1bbb93e` uses the revised diagnostic writer.
Both capture controllers are `DISABLED`, their run identifiers differ from the
preceding trial, and every present-rate window has `readback_enabled=0`.
No new image or FIFO prefix is attributed to this run. The operator confirms DBI
readiness afterward; no new visual/fluidity observation was supplied.

## Measured startup and diagnosed stop

| Window ending at presentation | Presentations in window | Duration | Rate |
| --- | --- | --- | --- |
| 48 | 47 | 5.058 s | 9.292 Hz |
| 86 | 38 | 5.096 s | 7.457 Hz |
| 93 | 7 | 17.366 s | 0.403 Hz |
| 99 | 6 | 5.501 s | 1.091 Hz |

The four windows average 2.968 completed presentations per second over 33.021 s.
The [preceding capture-disabled trial](HARDWARE_PRESENT_RATE_2026-10-09.md)
averages 1.176 Hz over a different, longer set of windows. The early presentation
rate increases after the SD sampling change, but loading and stalls remain.
Different endpoints and run durations prevent treating the ratio as a controlled
benchmark or gameplay FPS improvement.

The sampled post-main report records 102 successful presents with zero failures,
coherent main/fiber state and a structurally valid FST. Texture loading advances
through a later I4 16×16 object within the admitted descriptor family. The fresh
terminal report diagnoses an abort at 49.631 seconds:

- `IOS_IOCTL_KD_UNPROVEN_TUPLE`, target `IOS_Ioctl` / PAL `0x80194290`;
- an open `/dev/net/kd/request`, mode 0, handle 2004;
- command 2, with 32-byte input and output buffers;
- 641,998 translated dispatches, in a worker fiber after the earlier resume.

The previous close report belongs to handle 2003. The exception report is absent.
This is an attributable reason for this program termination. Pixel contents
through the preceding black interval remain unverified.

## Pinned semantics and bounded correction

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`,
`runtime/src/hle/net/network_config.cpp`, implements three try-suspend phases:

| Current phase and request | Guest result at output +0 | Next phase |
| --- | --- | --- |
| Boot, command 2 | −42 | Boot, boot probe recorded |
| Boot after boot probe, command 3 | 0 | PostResumeProbe |
| PostResumeProbe, command 2 | −42 | Ready |
| Ready, command 2 | 0 | Ready |

IOS itself returns zero for these admitted requests. The post-resume pending
reply is deliberate: reusing the preceding command-3 success can make the guest
SDK panic. Closing a request handle does not reset the process-wide phase.

The old Switch bridge tracks only handles 2000–2003 and refuses this later
command-2 request before implementing that transition. The correction tracks
up to 32 concurrent live request handles, allocates monotonically increasing
positive IDs, reuses closed table slots, and preserves scheduler phase across
close/reopen. It retains only the attributed commands 1, 2, 3 and 15, the mode-0
request path, and the observed 32-byte buffer shapes. Both complete declared
ranges are validated before any ioctl CPU, memory or phase mutation.

Command 1 preserves its zero result; command 15 preserves the pinned stable
user-ID reply and clears the entire declared output. Valid close releases only
its handle. Unknown commands, other nodes, malformed or unmapped buffers, closed
handles and the concurrent-handle limit remain diagnosed stops. No socket or
host-network operation is added. Ready-phase repeats and alternative handle
lifetimes have synthetic coverage; their console execution remains unproven.

The IOS entry traits now call a single implementation translation unit. Future
implementation changes can rebuild that bridge without rewriting an inline
body included by all generated guest shards. The first extraction still requires
a rebuild of affected shards.

## Validation and remaining work

Nintendo-data-free contracts invoke the actual three IOS traits and bridge in
rendered and headless modes, using the real Memory slice and synthetic backing.
They check the original four-request sequence, this fifth request, Ready-phase
repeats, resume-before-probe behavior, CPU preservation, independently specified
big-endian output bytes and canaries across complete memory, handle reuse, and
immediate refusals before mutation. Public CI includes these contracts.

The corrected KD request, its return to the caller and the next rendering stage
still require the exact rebuilt candidate on Switch. Raw reports, private NROs,
product fingerprints and recovery archives stay excluded from public source.
