#pragma once

// WiiCompiled's pinned AArch64 ISA header uses Clang's ext_vector_type for the
// two-word state-free result ABI. devkitA64 builds Switch homebrew with GCC,
// whose equivalent is vector_size. Keep the compatibility substitution scoped
// to translated-shard builds via Makefile -include; ordinary public builds do
// not need it.
#if defined(__GNUC__) && !defined(__clang__)
#ifndef ext_vector_type
#define ext_vector_type(N) vector_size(8 * (N))
#endif
#endif
