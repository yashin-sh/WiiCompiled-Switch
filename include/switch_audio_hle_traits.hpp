#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstring>

namespace mkw::switch_audio_hle {

inline std::uint32_t g_ai_dma_start_addr = 0u;
inline std::uint32_t g_ai_dma_register_start_addr = 0u;
inline std::uint32_t g_ai_dma_length = 0u;
inline std::uint32_t g_ai_dma_bytes_left = 0u;
inline bool g_ai_dma_enabled = false;
inline std::uint32_t g_ai_dma_sample_rate = 32000u;

inline constexpr std::uint32_t EncodeAIDmaStartRegister(std::uint32_t startAddr) noexcept {
    return startAddr & 0x1FFFFFE0u;
}

inline constexpr std::uint32_t EncodeAIDmaLengthRegister(std::uint32_t length) noexcept {
    return length & 0x000FFFE0u;
}

inline constexpr float ClampSoundPlayerVolume(float volume) noexcept {
    if (volume <= 1.0f) {
        return volume < 0.0f ? 0.0f : volume;
    }
    return 1.0f;
}

inline std::uint32_t FloatBits(float value) noexcept {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

} // namespace mkw::switch_audio_hle

// Early Wii audio bootstrap is a native/HLE boundary in the pinned runtime.
// The desktop implementation starts the host audio backend and AX/DSP emulation,
// neither of which is required to prove boot-to-main on Horizon. Keep these
// entry points as deliberate guest-CPU no-ops for the fast-track; the real
// Switch audio backend will replace this boundary after first-frame bring-up.
//
// PAL addresses from pinned WiiCompiled a135beb...:
//   0x80123F88 AIRegisterDMACallback
//   0x80123FCC AIInitDMA
//   0x80124048 AIStartDMA
//   0x801240B0 AIInit
//   0x801269BC __AXOutInitDSP
//   0x801A1138 __AIClockInit
//   0x801A1358 __OSInitAudioSystem
//   0x801A1520 __OSStopAudioSystem

// Real-Switch hardware crossed OSSetPeriodicAlarm and then reached
// nw4r::snd::SoundPlayer::SetVolume (PAL 0x800A35E0) while loading the real
// revo_kart.brsar sound archive. PPC EABI carries the scalar float in f1.
// Pinned WiiCompiled clamps that value to [0, 1] with its original NaN
// behavior, then writes the resulting float to SoundPlayer + 0x2C. Its media
// attenuation layer is host-only policy; do not reproduce it on Horizon at
// this blocker, and do not pre-port neighboring NW4R sound APIs.
template <>
struct KnownNativeCpuCall<0x800A35E0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kVolumeOffset = 0x2Cu;
        const std::uint32_t soundPlayer = cpu->gpr[3];
        const float requested = static_cast<float>(cpu->fpr[1].d);
        const float clamped = mkw::switch_audio_hle::ClampSoundPlayerVolume(requested);

        try {
            if (soundPlayer == 0u ||
                !Memory::Contains(soundPlayer + kVolumeOffset, sizeof(std::uint32_t))) {
                return;
            }
            Memory::Write32(
                soundPlayer + kVolumeOffset,
                mkw::switch_audio_hle::FloatBits(clamped));
        } catch (...) {
            // Keep guest-memory faults contained at the native HLE boundary.
        }
    }
};

// Real-Switch hardware crossed __AXOutInitDSP and then reached PAL
// AIRegisterDMACallback (0x80123F88) with callback=0x80126898. Pinned
// WiiCompiled stores the callback in the guest global at 0x80386480 and returns
// the previous callback pointer. Mirror only that guest-visible contract; host
// callback dispatch remains unsupported until hardware proves it is required.
template <>
struct KnownNativeCpuCall<0x80123F88u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kAIDmaCallbackAddr = 0x80386480u;
        const std::uint32_t callback = cpu->gpr[3];
        std::uint32_t oldCallback = 0u;

        try {
            if (Memory::Contains(kAIDmaCallbackAddr, 4u)) {
                oldCallback = Memory::Read32(kAIDmaCallbackAddr);
                Memory::Write32(kAIDmaCallbackAddr, callback);
            }
        } catch (...) {
            // Pinned WiiCompiled uses TryRead32/TryWrite32 here. Failed guest
            // access therefore preserves the default old-callback value of 0.
        }

        cpu->gpr[3] = oldCallback;
    }
};

// Real-Switch hardware crossed AIRegisterDMACallback and then reached PAL
// AIInitDMA (0x80123FCC) with start=0x802F7D20 and length=0x180. Pinned
// WiiCompiled updates only shared host-side AI DMA state at this boundary. Keep
// that exact state so later hardware-proven AI calls can observe it, without
// starting DMA, touching guest memory, or constructing a Horizon audio backend.
template <>
struct KnownNativeCpuCall<0x80123FCCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t startAddr = cpu->gpr[3];
        const std::uint32_t length = cpu->gpr[4];

        mkw::switch_audio_hle::g_ai_dma_start_addr = startAddr;
        mkw::switch_audio_hle::g_ai_dma_register_start_addr =
            mkw::switch_audio_hle::EncodeAIDmaStartRegister(startAddr);
        mkw::switch_audio_hle::g_ai_dma_length =
            mkw::switch_audio_hle::EncodeAIDmaLengthRegister(length);
        mkw::switch_audio_hle::g_ai_dma_bytes_left =
            mkw::switch_audio_hle::g_ai_dma_length;
    }
};

// Real-Switch hardware crossed AIInitDMA and then reached PAL AIStartDMA
// (0x80124048). Pinned WiiCompiled sets the default 32 kHz sample rate, marks
// DMA enabled, and reloads bytesLeft from the already-initialized DMA length.
// The desktop runtime also starts its host audio backend here; do not invent a
// Horizon backend at this hardware boundary.
template <>
struct KnownNativeCpuCall<0x80124048u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext*) noexcept {
        mkw::switch_audio_hle::g_ai_dma_sample_rate = 32000u;
        mkw::switch_audio_hle::g_ai_dma_enabled = true;
        mkw::switch_audio_hle::g_ai_dma_bytes_left =
            mkw::switch_audio_hle::g_ai_dma_length;
    }
};

// Real-Switch hardware reached PAL AIInit (0x801240B0) with r3=0 after the
// fourth KD request close was crossed. Pinned WiiCompiled initializes the
// guest-visible AI globals on the first call and separately starts a desktop
// host audio backend. Mirror only the guest state here: Horizon audio remains
// outside this exact blocker, and adjacent AI entry points stay unsupported
// until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x801240B0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kAIInitializedAddr = 0x80386448u;
        constexpr std::uint32_t kAICallbackBusyAddr = 0x8038644Cu;
        constexpr std::uint32_t kAICallbackStackSwitchAddr = 0x8038647Cu;
        constexpr std::uint32_t kAIDmaCallbackAddr = 0x80386480u;

        try {
            if (!Memory::Contains(kAIInitializedAddr, 4u) ||
                !Memory::Contains(kAICallbackBusyAddr, 4u) ||
                !Memory::Contains(kAICallbackStackSwitchAddr, 4u) ||
                !Memory::Contains(kAIDmaCallbackAddr, 4u)) {
                return;
            }

            if (Memory::Read32(kAIInitializedAddr) == 1u) {
                return;
            }

            Memory::Write32(kAIDmaCallbackAddr, 0u);
            Memory::Write32(kAICallbackBusyAddr, 0u);
            Memory::Write32(kAICallbackStackSwitchAddr, cpu->gpr[3]);
            Memory::Write32(kAIInitializedAddr, 1u);
        } catch (...) {
            // Pinned WiiCompiled uses TryRead/TryWrite for this bookkeeping;
            // an unavailable guest range therefore leaves AIInit best-effort.
        }
    }
};

// Real-Switch hardware crossed AIInit and then reached PAL __AXOutInitDSP
// (0x801269BC). Pinned WiiCompiled initializes host AX/DSP state and publishes
// the guest DSP task contract. Mirror only those guest-visible writes here;
// the host mix worker, mail queue, and audio backend remain unimplemented until
// hardware reaches a boundary that proves they are required.
template <>
struct KnownNativeCpuCall<0x801269BCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kAxDspTaskAddr = 0x802F81A0u;
        constexpr std::uint32_t kDspInitializedAddr = 0x80386608u;
        constexpr std::uint32_t kDspAssertPendingAddr = 0x80386610u;
        constexpr std::uint32_t kDspAssertTaskAddr = 0x80386614u;
        constexpr std::uint32_t kDspUnknownStateAddr = 0x80386618u;
        constexpr std::uint32_t kDspCurrentTaskAddr = 0x8038661Cu;
        constexpr std::uint32_t kDspFirstTaskAddr = 0x80386620u;
        constexpr std::uint32_t kDspRunningTaskAddr = 0x80386624u;
        constexpr std::uint32_t kAxIramMmemAddr = 0x8027F820u;
        constexpr std::uint32_t kAxDramMmemAddr = 0x802F8200u;
        constexpr std::uint32_t kAxDramLength = 64u;
        constexpr std::uint32_t kAxDramDspAddr = 3282u;
        constexpr std::uint32_t kAxInitCallback = 0x80126948u;
        constexpr std::uint32_t kAxResumeCallback = 0x80126954u;
        constexpr std::uint32_t kAxDoneCallback = 0x801269A8u;
        constexpr std::uint32_t kAxRequestCallback = 0x801269B8u;

        const std::uint32_t r13 = cpu->gpr[13];
        if (r13 < 0x7400u) {
            return;
        }

        const std::uint32_t sdaInput = r13 - 0x7400u;
        const std::uint32_t sdaOutput = r13 - 0x66DCu;

        try {
            if (!Memory::Contains(kAxDspTaskAddr, 0x40u) ||
                !Memory::Contains(kDspInitializedAddr, 0x20u) ||
                !Memory::Contains(sdaInput, 6u) || !Memory::Contains(sdaOutput, 8u)) {
                return;
            }

            Memory::Write32(kDspInitializedAddr, 1u);
            Memory::Write32(kDspAssertPendingAddr, 0u);
            Memory::Write32(kDspAssertTaskAddr, 0u);
            Memory::Write32(kDspUnknownStateAddr, 0u);
            Memory::Write32(kDspCurrentTaskAddr, 0u);
            Memory::Write32(kDspFirstTaskAddr, 0u);
            Memory::Write32(kDspRunningTaskAddr, 0u);

            Memory::Write32(kAxDspTaskAddr + 0x00u, 1u);
            Memory::Write32(kAxDspTaskAddr + 0x04u, 0u);
            Memory::Write32(kAxDspTaskAddr + 0x0Cu, kAxIramMmemAddr);
            Memory::Write32(kAxDspTaskAddr + 0x10u, Memory::Read16(sdaInput + 0x04u));
            Memory::Write32(kAxDspTaskAddr + 0x14u, 0u);
            Memory::Write32(kAxDspTaskAddr + 0x18u, kAxDramMmemAddr);
            Memory::Write32(kAxDspTaskAddr + 0x1Cu, kAxDramLength);
            Memory::Write32(kAxDspTaskAddr + 0x20u, kAxDramDspAddr);
            Memory::Write16(kAxDspTaskAddr + 0x24u, Memory::Read16(sdaInput + 0x00u));
            Memory::Write16(kAxDspTaskAddr + 0x26u, Memory::Read16(sdaInput + 0x02u));
            Memory::Write32(kAxDspTaskAddr + 0x28u, kAxInitCallback);
            Memory::Write32(kAxDspTaskAddr + 0x2Cu, kAxResumeCallback);
            Memory::Write32(kAxDspTaskAddr + 0x30u, kAxDoneCallback);
            Memory::Write32(kAxDspTaskAddr + 0x34u, kAxRequestCallback);
            Memory::Write32(kAxDspTaskAddr + 0x38u, 0u);
            Memory::Write32(kAxDspTaskAddr + 0x3Cu, 0u);

            Memory::Write32(kDspCurrentTaskAddr, kAxDspTaskAddr);
            Memory::Write32(kDspFirstTaskAddr, kAxDspTaskAddr);
            Memory::Write32(kDspRunningTaskAddr, kAxDspTaskAddr);
            Memory::Write32(sdaOutput + 0x04u, 1u);
            Memory::Write32(sdaOutput + 0x00u, 0u);
        } catch (...) {
            // Keep the Switch fast-track best-effort if guest memory is not
            // available; the real hardware path maps every range above.
        }
    }
};

template <>
struct KnownNativeCpuCall<0x801A1138u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

template <>
struct KnownNativeCpuCall<0x801A1358u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

template <>
struct KnownNativeCpuCall<0x801A1520u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};
