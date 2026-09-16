# PAL PADInit — 0x801AF2F0

Real Switch hardware exposed `0x801AF2F0` as the next direct blocker after `WPADControlMotor` was crossed.

At pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`, this is `PAD__Init_HLE()`: it calls Aurora `PADInit()` and returns `1` on success. Aurora's pinned implementation is idempotent, marks PAD initialized, seeds desktop keyboard mappings, and returns true.

The Switch bridge preserves only the proven initialization state and guest success return. It does not construct SDL/Aurora input devices or pre-port other PAD entry points.
