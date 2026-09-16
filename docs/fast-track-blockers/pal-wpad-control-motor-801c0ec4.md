# PAL fast-track blocker — WPADControlMotor (`0x801C0EC4`)

Real Switch hardware reached this direct boundary after #149 crossed `WPADGetStatus`.

Pinned WiiCompiled semantics at `a135beb201042b20f390c6695ca6b26768820fb4`: `WPADControlMotor_HLE(chan, command)` is a `void` no-op. Both arguments are ignored; no guest memory, callbacks, device state, Bluetooth state, or rumble backend is touched.

Switch action: expose only `KnownNativeCpuCall<0x801C0EC4>` as a register-preserving no-op, with Nintendo-data-free compile/link coverage. Do not pre-port any other WPAD behavior.
