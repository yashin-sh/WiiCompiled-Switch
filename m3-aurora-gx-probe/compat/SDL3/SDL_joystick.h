// Minimal SDL3 compilation shim for the Switch probe builds.
// See SDL_stdinc.h for the rationale (types only, no behavior).

#ifndef M3_SDL3_SHIM_JOYSTICK_H
#define M3_SDL3_SHIM_JOYSTICK_H

#include <stdint.h>

typedef uint32_t SDL_JoystickID;

#endif // M3_SDL3_SHIM_JOYSTICK_H
