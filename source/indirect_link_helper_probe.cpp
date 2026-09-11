// Nintendo-data-free link coverage for the helper families that became reachable
// when the fast-track started linking WiiCompiled's generated base_dispatch.
// Taking the addresses in a static constructor keeps the references alive through
// --gc-sections without executing any guest-memory or GX helper.
#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

#include "isa/ppc_isa_float.h"
#include "isa/ppc_isa_int.h"
#include "isa/ppc_isa_quantized.h"
#include "ppc_runtime.h"

#include <cstdint>

extern "C" void GX_HLE_FIFO_Write8(std::uint8_t value);
extern "C" void GX_HLE_FIFO_Write16(std::uint16_t value);
extern "C" void GX_HLE_FIFO_Write32(std::uint32_t value);
extern "C" void GX_HLE_FIFO_WriteFloat(float value);
extern "C" void GX_HLE_FIFO_WriteBurst(const std::uint8_t* data, std::uint32_t sizeBytes);
extern "C" void PPCMfhid2_HLE_8012e630(CpuContext* ctx);

namespace {
struct IndirectLinkHelperProbe {
    IndirectLinkHelperProbe() {
        volatile auto ppcPsqLoad = &PPC_PsqL;
        volatile auto ppcPsqStore = &PPC_PsqSt;
        volatile auto fifoWrite8 = &GX_HLE_FIFO_Write8;
        volatile auto fifoWrite16 = &GX_HLE_FIFO_Write16;
        volatile auto fifoWrite32 = &GX_HLE_FIFO_Write32;
        volatile auto fifoWriteFloat = &GX_HLE_FIFO_WriteFloat;
        volatile auto fifoWriteBurst = &GX_HLE_FIFO_WriteBurst;
        volatile auto storeHalfwordByteReverse = &PPC_StoreHalfwordByteReverse;
        volatile auto memsetZero32 = &memset_zero_32;
        volatile auto trapWord = &PPC_TrapWord;
        volatile auto loadWordByteReverse = &PPC_LoadWordByteReverse;
        volatile auto osSystemCall = &OSSystemCall;
        volatile auto storeWordByteReverse = &PPC_StoreWordByteReverse;
        volatile auto psSel = &PPC_PsSel;
        volatile auto mtfsf = &PPC_Mtfsf;
        volatile auto mffs = &PPC_Mffs;
        volatile auto mfhid2 = &PPCMfhid2_HLE_8012e630;

        (void)ppcPsqLoad;
        (void)ppcPsqStore;
        (void)fifoWrite8;
        (void)fifoWrite16;
        (void)fifoWrite32;
        (void)fifoWriteFloat;
        (void)fifoWriteBurst;
        (void)storeHalfwordByteReverse;
        (void)memsetZero32;
        (void)trapWord;
        (void)loadWordByteReverse;
        (void)osSystemCall;
        (void)storeWordByteReverse;
        (void)psSel;
        (void)mtfsf;
        (void)mffs;
        (void)mfhid2;
    }
};

IndirectLinkHelperProbe g_indirectLinkHelperProbe;
} // namespace

#endif
