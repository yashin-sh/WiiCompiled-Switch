# Fast-track blocker: PAL OSReport (0x801A25D0)

Hardware record:

- dispatch kind: `DIRECT`
- target: `0x801A25D0`
- guest PC: `0x800060A4`
- stage: `TRANSLATED_EXEC_ENTER`

Pinned WiiCompiled (`a135beb201042b20f390c6695ca6b26768820fb4`) registers PAL `0x801A25D0` as a native `OSReport` override. Its implementation reads/formats guest arguments and emits host logging only; it does not modify `CpuContext` or guest memory.

For the Switch fast-track, this boundary is therefore represented as a guest-state-preserving native HLE sink rather than importing the desktop printf/logging stack. Public synthetic coverage invokes the same static native-dispatch path with a null synthetic format pointer and contains no Nintendo data.
