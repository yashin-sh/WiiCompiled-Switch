// Minimal SDL3 compilation shim for the Switch probe builds.
// See SDL_stdinc.h for the rationale (types only, no behavior).
//
// Only the leading event-type discriminator and the overall size (56 bytes,
// matching real SDL_Event) are reproduced. No probe code reads event
// payloads at runtime.

#ifndef M3_SDL3_SHIM_EVENTS_H
#define M3_SDL3_SHIM_EVENTS_H

#include <stdint.h>

#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

typedef struct SDL_Event {
    Uint32 type;
    uint8_t reserved[52];
} SDL_Event;

#endif // M3_SDL3_SHIM_EVENTS_H
