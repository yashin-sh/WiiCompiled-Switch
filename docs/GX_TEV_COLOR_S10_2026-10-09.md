# Signed TEV color bridge — 2026-10-09

The [verified console trial](HARDWARE_TEX_COORD_TEV_S10_2026-10-09.md)
stops at the missing direct target `GXSetTevColorS10` (`0x80171E70`),
register ID 1 and guest pointer `0x80398E80`. The report does not record
its color components or establish whether those eight bytes are readable.

The source-owned extension registry now resolves this target to a rendered
bridge. It checks the full 32-bit register ID against the four native TEV
registers, then requires a complete eight-byte mapped guest range. It decodes
big-endian signed 16-bit R/G/B/A components and forwards them to the pinned
`GXSetTevColorS10` writer. It preserves all input encodings: the native writer
masks each component to eleven bits instead of clamping to the SDK's named
range. Mapped zero and unaligned addresses are allowed by the existing memory
contract; overflow and partial mappings are refused before native output.

The actual pinned writer emits two BP packets: R/A in the even register and
B/G in the following odd register, selected from `0xE0..0xE7`. It sets native
`bpSent` and adds no guest-memory mirror. Invalid IDs, unreadable ranges and
headless execution remain diagnostic stops. The shared ABI header, generated
translations and original private integration patches are unchanged.

`bash scripts/test-gx-light-object.sh` passes with ASan/UBSan. The new contract
checks 196,610 actual writer/decoder packet cases, including every 16-bit
encoding in each component, all four register IDs, unaligned inputs, exact
memory ends, mapped zero and the top of the 32-bit address space. Independent
byte and signed-value oracles check register ordering, eleven-bit masking and
sign extension. Complete CPU, guest-memory and native-state canaries check
preservation; decoder canaries also protect the other TEV and K-color registers.
The suite retains light, normal-matrix, depth-texture, dispatch priority and
public-probe contracts. The decoder uses a small state seam; it proves these
register effects, without claiming GPU pixels or a console return.

The separate private rendered/capture build, SDK syntax, scoped provider and
ELF-retention audits, and all published checks on the exact PR HEAD must pass
before merge. The resulting NRO must be checked by complete SD readback before
the next console trial. Hardware acceptance remains pending that fresh run.

The initial public SDK jobs hit Docker Hub rate limits before compilation.
The [CI image correction](CI_MERGE_POLICY_2026-10-09.md#sdk-image-availability)
pins a byte-verified cached copy of the official image; all checks still run.
