# Scissor origin returned; light-object boundary — 2026-10-09

The Switch trial of PR #344 code `c0fb727` advances past the scissor-origin
boundary. Its fresh terminal stage records `GXSetScissorBoxOffset` entry,
followed by a later `DIRECT` stop at `GXLoadLightObjImm`, PAL `0x80170320`,
after 55.219 seconds and 648,157 translated dispatches. This accepts the
observed scissor helper returning; other coordinates and pixel correctness
remain covered by synthetic contracts rather than new hardware images.

The light-object call has guest pointer `0x802B90B8` and light ID `1`.
The local caller loads lights in a conditional slot loop. Its recorded link
register survives an earlier call and does not identify the precise instruction
of this later direct call. No light data or native light operation has yet been
observed on Switch.

Both capture controllers have fresh identifiers and report `DISABLED`. All
presentation windows have readback off. The 39 retrieved reports total 805,229
bytes; their per-file comparison is retained privately. The exception report is
absent. The sampled main-thread snapshot is coherent, its FST is structurally
valid, and it records 102 successful presents with zero failures. These counters
do not establish new visible pixels. The user confirmed DBI/USB readiness without
a new screen or fluidity observation.

| Presentations in window | Duration | Rate |
| --- | --- | --- |
| 46 | 5.015 s | 9.172 Hz |
| 38 | 6.201 s | 6.128 Hz |
| 10 | 11.781 s | 0.849 Hz |
| 5 | 11.745 s | 0.426 Hz |

The windows average 2.850 Hz over 34.742 seconds. They include loading and stalls;
these are not gameplay FPS or a controlled speed comparison. The earlier
[recognizable boot image and desktop replay](HARDWARE_NONBLACK_REPLAY_2026-10-09.md)
remain the latest pixel evidence.

## Light-object correction

The bridge converts a complete mapped, nonwrapping 64-byte guest object into
Aurora's distinct host layout. It reads big-endian color at +12 and twelve floats
at +16, then initializes attenuation, position and direction through the pinned
native functions. Guest direction already uses XF signs, so the initializer's
negation is compensated. Native load emits a 16-word XF light packet with zero
padding. All eight one-bit IDs are supported; zero, multibit and larger masks
refuse before native output. Invalid object ranges likewise refuse. CPU and guest
memory are preserved; headless execution diagnoses the missing renderer.

A source-owned extension registry handles only missing direct-call targets,
after the existing native and translated constexpr paths. Known calls retain
priority and avoid the lookup. Unknown targets keep their diagnostic stop;
indirect dispatch is unchanged. Future additions to this registry need no ABI
header edit. A comparison of native Ninja dependency plans confirms that changing the
registry adds one source compilation and no generated-shard rebuild. It does
not measure a wall-clock speedup.

The public contract executes the actual five pinned Aurora function bodies and
compares their complete FIFO bytes with an independent guest-word fixture. It
covers all IDs, unaligned and end-of-region objects, distinct colors, signed zero,
quiet NaNs, infinities and subnormals, plus CPU/memory canaries and diagnosed
refusals before output. A separate dispatch contract checks both fast-path
priorities and exactly one runtime-options invocation for extensions. The probe
registry is separately compiled without translated headers or execution flags
and exposes no execution handlers. The existing build workflow runs these contracts. Light candidate code `c2d862a` passes the
private rendered build, SDK syntax and scoped provider/retention checks. Its
complete published CI rollup is now 12/12 successful after interrupted runs were
[rerun](CI_MERGE_POLICY_2026-10-09.md). The light operation's hardware return
remains pending.

Raw reports, game translations, NROs and image/replay data remain local.
