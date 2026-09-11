// Temporary Switch fast-track sink for WiiCompiled's GX FIFO helpers.
//
// Linking base_dispatch makes many translated GX SDK routines reachable before
// the real GX->Switch renderer exists. The desktop runtime forwards these calls
// to HleFifoWrite/Aurora; importing that backend here would pull a large desktop
// dependency graph. For boot-to-main, consume the FIFO writes without touching
// Wii MMIO. This file must be replaced by the real graphics backend before first
// frame validation.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include <cstdint>

extern "C" void GX_HLE_FIFO_Write8(std::uint8_t value) {
    (void)value;
}

extern "C" void GX_HLE_FIFO_Write16(std::uint16_t value) {
    (void)value;
}

extern "C" void GX_HLE_FIFO_Write32(std::uint32_t value) {
    (void)value;
}

extern "C" void GX_HLE_FIFO_WriteFloat(float value) {
    (void)value;
}

extern "C" void GX_HLE_FIFO_WriteBurst(const std::uint8_t* data, std::uint32_t sizeBytes) {
    (void)data;
    (void)sizeBytes;
}

#endif
