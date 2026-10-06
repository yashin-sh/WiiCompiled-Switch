#include <dolphin/gx.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

#define CHECK(condition, ...) assert(condition)
#include "pinned-copy-size.inc"
#undef CHECK

int main() {
    unsigned cases = 0;
    for (unsigned width = 1; width <= 255; ++width)
        for (unsigned height = 1; height <= 255; ++height) {
            // Independent wire expectation: RGB5A3 stores 4x4 tiles of 32 bytes.
            const auto expected = ((width + 3u) / 4u) * ((height + 3u) / 4u) * 32u;
            assert(GXGetTexBufferSize(width, height, GX_TF_RGB5A3, GX_FALSE, 0) == expected);
            ++cases;
        }
    assert(GXGetTexBufferSize(128, 128, GX_TF_RGB5A3, GX_FALSE, 0) == 32768u);
    std::printf("PASS: pinned RGB5A3 copy-size cases=%u\n", cases);
}
