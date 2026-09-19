// Minimal SDL3 compilation shim for the Switch probe builds.
//
// Aurora's public headers reference a small set of SDL3 *types* (event and
// window handles). No SDL function is ever called by the translation units
// compiled into the Switch probes, and no SDL library is linked. These
// headers provide just enough declarations to compile that subset.
//
// This is NOT a reimplementation of SDL3 behavior: SDL_Event carries no
// meaningful payload here and must not cross any runtime boundary.

#ifndef M3_SDL3_SHIM_STDINC_H
#define M3_SDL3_SHIM_STDINC_H

#include <stdint.h>

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int64_t Sint64;

typedef int SDL_bool;
#define SDL_TRUE 1
#define SDL_FALSE 0

#endif // M3_SDL3_SHIM_STDINC_H
