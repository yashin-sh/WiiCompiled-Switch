// GX handlers covered here link against their actual pinned native bodies.
// Other providers have fail-on-execution link seams and separate contracts.
#include "pinned-light-object.inc"
#include "pinned-gx-data.inc"
__GXData_struct nativeState{};
__GXData_struct* __gx = &nativeState;
#define GX_WRITE_U8(value) Emit(1, static_cast<std::uint8_t>(value))
#define GX_WRITE_U32(value) Emit(4, static_cast<std::uint32_t>(value))
#define GX_WRITE_F32(value) Emit(4, std::bit_cast<std::uint32_t>(value))
#define GX_WRITE_RAS_REG(value)                     \
    do {                                            \
        Emit(1, 0x61);                              \
        Emit(4, static_cast<std::uint32_t>(value)); \
    } while (false)
#define CHECK(condition, ...) assert(condition)
#include "pinned-set-reg-field.inc"
extern "C" {
#include "pinned-light-native.inc"
#include "pinned-normal-native.inc"
#include "pinned-z-texture-native.inc"
#include "pinned-tev-s10-native.inc"
}
