# PAL WPADGetDpdSensitivity — 0x801C329C

Real Switch hardware reached this direct native boundary after #147 crossed `WPADInit`.

Pinned WiiCompiled maps `0x801C329C` to `WPADGetDpdSensitivity_HLE`, which returns the shared WPAD stub state's `dpdSensitivity` value. That state defaults to `3`.

The Switch fast-track mirrors only this observed getter: shared default state `3`, returned through guest `r3`. No controller probing or other WPAD behavior is included.
