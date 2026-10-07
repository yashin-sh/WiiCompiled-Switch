#include "capture.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace replay {
namespace {
Recorder* active = nullptr;
constexpr std::array<std::uint8_t, 8> Magic{'M', 'K', 'W', 'R', 'P', 'L', '1', 0};
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
std::uint64_t read(std::span<const std::uint8_t> b, std::size_t p, std::size_t n) {
    require(p <= b.size() && n <= b.size() - p, "truncated replay or FIFO command");
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < n; ++i) {
        value = (value << 8) | b[p + i];
    }
    return value;
}
void write(Bytes& b, std::size_t p, std::uint64_t value, std::size_t n) {
    require(p <= b.size() && n <= b.size() - p, "invalid relocation offset");
    for (std::size_t i = 0; i < n; ++i) {
        b[p + n - 1 - i] = static_cast<std::uint8_t>(value);
        value >>= 8;
    }
}
void integer(Bytes& b, std::uint32_t value) {
    const auto offset = b.size();
    b.resize(offset + 4);
    write(b, offset, value, 4);
}
std::uint32_t checksum(std::span<const std::uint8_t> b, Kind kind) {
    auto value = 2166136261u ^ static_cast<std::uint32_t>(kind);
    for (auto byte : b) {
        value = (value ^ byte) * 16777619u;
    }
    return value;
}
std::size_t texture_size(std::uint32_t width, std::uint32_t height, std::uint32_t format) {
    require(width > 0 && width <= 1024 && height > 0 && height <= 1024, "unsupported texture dimensions");
    // GX tiled blocks: I4/C4/CMPR, I8/IA4/C8, IA8/RGB565/RGB5A3/C14X2, RGBA8.
    std::uint32_t bw = 4, bh = 4, block = 32;
    switch (format) {
    case 0:
    case 8:
    case 14:
        bw = 8;
        bh = 8;
        break;
    case 1:
    case 2:
    case 9:
        bw = 8;
        break;
    case 3:
    case 4:
    case 5:
    case 10:
        break;
    case 6:
        block = 64;
        break;
    default:
        throw std::runtime_error("unsupported texture format");
    }
    return ((width + bw - 1) / bw) * ((height + bh - 1) / bh) * block;
}
} // namespace

void Decoder::relocate(Bytes& fifo, const Relocate& resolve) {
    std::size_t pos = 0;
    while (pos < fifo.size()) {
        const auto opcode = fifo[pos];
        std::size_t length = 1;
        if (opcode == 0 || opcode == 0x48) {
            // NOP / invalidate vertex cache.
        } else if (opcode == 0x61) {
            length = 5;
            const auto reg = read(fifo, pos + 1, 1);
            require(reg != 0x4B && reg != 0x52, "EFB copy commands are outside replay v1");
        } else if (opcode == 0x08) {
            length = 6;
            const auto reg = read(fifo, pos + 1, 1);
            const auto value = static_cast<std::uint32_t>(read(fifo, pos + 2, 4));
            require((reg & 0xF0) != 0xA0, "guest CP array bases are outside replay v1");
            if (reg == 0x50) {
                vcdLo = value;
            } else if (reg == 0x60) {
                vcdHi = value;
            } else if (reg >= 0x70 && reg <= 0x77) {
                vat[reg - 0x70] = value;
            }
        } else if (opcode == 0x10) {
            const auto header = read(fifo, pos + 1, 4);
            length = 5 + ((header >> 16) + 1) * 4;
        } else if (opcode == 0x50) {
            const auto sub = read(fifo, pos + 1, 2);
            std::size_t pointer = 0, needed = 0;
            if (sub == 1) {
                length = 27;
            } else if (sub == 2) {
                length = 19;
            } else if (sub >= 0x10 && sub <= 0x1F) {
                length = 16;
                pointer = pos + 3;
                needed = read(fifo, pos + 11, 4);
                require(needed > 0 && needed <= MaxBytes && read(fifo, pos + 15, 1) <= 1, "invalid array metadata");
            } else if (sub == 0x30) {
                length = 37;
                require(read(fifo, pos + 3, 1) < 8, "invalid texture slot");
                pointer = pos + 4;
                needed = texture_size(read(fifo, pos + 12, 4), read(fifo, pos + 16, 4), read(fifo, pos + 20, 4));
                require(read(fifo, pos + 24, 4) < 20 && read(fifo, pos + 28, 1) == 0, "mipmapped texture or invalid TLUT slot");
            } else if (sub == 0x31) {
                length = 26;
                require(read(fifo, pos + 3, 1) < 20 && read(fifo, pos + 12, 4) <= 2, "invalid palette metadata");
                pointer = pos + 4;
                needed = read(fifo, pos + 16, 2) * 2;
                require(needed > 0 && needed <= 32768, "invalid palette size");
            } else if (sub == 0x32 || sub == 0x33) {
                length = 7;
            } else if (sub == 0x35 || sub == 0x21) {
                length = 3;
            } else if (sub == 0x20 || sub == 0x22) {
                length = 5 + read(fifo, pos + 3, 2);
            } else {
                throw std::runtime_error("unsupported Aurora subcommand");
            }
            require(length <= fifo.size() - pos, "truncated Aurora command");
            if (pointer) {
                write(fifo, pointer, resolve(read(fifo, pointer, 8), needed), 8);
            }
        } else if (opcode >= 0x80 && opcode <= 0xBF) {
            const auto primitive = opcode & 0xF8;
            require(primitive == 0x80 || primitive == 0x90 || primitive == 0x98 || primitive == 0xA0, "unsupported primitive");
            const auto a = vat[opcode & 7];
            require((vcdLo & 0x1FFF) == (1u << 9) && (vcdLo >> 15) == 0 && (vcdHi & ~3u) == 0, "unsupported vertex descriptor");
            require((a & 0x1FF) == 9, "position must be XYZ/F32 with zero fraction");
            std::size_t stride = 12;
            const auto color = (vcdLo >> 13) & 3;
            require(color <= 1, "indexed color is outside replay v1");
            if (color) {
                require(((a >> 13) & 0xF) == 11, "color must be RGBA8");
                stride += 4;
            }
            require((vcdHi & 3) <= 1, "indexed UV is outside replay v1");
            if (vcdHi & 3) {
                require(((a >> 21) & 0x1FF) == 9, "UV must be ST/F32 with zero fraction");
                stride += 8;
            }
            const auto count = read(fifo, pos + 1, 2);
            require(count >= 3 && count <= 4096 && (primitive != 0x80 || count % 4 == 0) && (primitive != 0x90 || count % 3 == 0), "invalid vertex count");
            length = 3 + count * stride;
        } else {
            throw std::runtime_error("unsupported FIFO opcode (including indexed XF and nested lists)");
        }
        require(length <= fifo.size() - pos, "truncated FIFO payload");
        pos += length;
    }
}

void Recorder::append(Kind kind, Bytes payload) {
    require(!ended, "capture already ended");
    require(payload.size() <= MaxBytes - total && 12 <= MaxBytes - total - payload.size(), "capture exceeds 8 MiB bound");
    total += payload.size() + 12;
    events.push_back({kind, std::move(payload)});
}
void Recorder::memory(std::span<const std::uint8_t> data) {
    require(!ended && !data.empty() && data.size() <= MaxBytes && ranges.size() < 64, "invalid capture memory registration");
    for (const auto& range : ranges) {
        require(range.data != data.data(), "duplicate capture memory registration");
    }
    ranges.push_back({data.data(), data.size()});
}
void Recorder::drain(std::span<const std::uint8_t> data) {
    require(!ended && !data.empty() && data.size() <= MaxBytes, "invalid capture drain");
    Bytes fifo(data.begin(), data.end());
    decoder.relocate(fifo, [&](std::uint64_t address, std::size_t needed) -> std::uint64_t {
        for (std::size_t i = 0; i < ranges.size(); ++i) {
            if (address == reinterpret_cast<std::uintptr_t>(ranges[i].data) && needed <= ranges[i].size) {
                return i + 1;
            }
        }
        throw std::runtime_error("FIFO references unregistered or undersized memory");
    });
    // Snapshot registered live data immediately before the consumer processes this
    // batch, including data referenced by slots loaded in an earlier batch.
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        require(total <= MaxBytes - 16 && ranges[i].size <= MaxBytes - total - 16, "capture memory exceeds bound");
        Bytes payload;
        integer(payload, i + 1);
        payload.insert(payload.end(), ranges[i].data, ranges[i].data + ranges[i].size);
        append(Kind::Memory, std::move(payload));
    }
    append(Kind::Fifo, std::move(fifo));
}
void Recorder::begin() {
    require(!begun && !events.empty(), "capture needs initial state before one frame");
    append(Kind::Begin, {});
    begun = true;
}
void Recorder::end() {
    require(begun && !events.empty() && events.back().kind == Kind::Fifo, "frame must end after a FIFO batch");
    append(Kind::End, {});
    ended = true;
}
Bytes Recorder::finish() const {
    require(ended, "incomplete capture cannot be saved");
    Bytes result(Magic.begin(), Magic.end());
    result.insert(result.end(), WiiPin, WiiPin + 40);
    result.insert(result.end(), DawnPin, DawnPin + 40);
    integer(result, 256);
    integer(result, 256);
    for (const auto& event : events) {
        integer(result, static_cast<std::uint32_t>(event.kind));
        integer(result, event.payload.size());
        integer(result, checksum(event.payload, event.kind));
        result.insert(result.end(), event.payload.begin(), event.payload.end());
    }
    return result;
}

Playback::Playback(std::span<const std::uint8_t> file) {
    require(file.size() >= 96 && file.size() <= MaxBytes, "invalid replay size");
    require(std::equal(Magic.begin(), Magic.end(), file.begin()), "invalid replay magic/version");
    require(std::memcmp(file.data() + 8, WiiPin, 40) == 0 && std::memcmp(file.data() + 48, DawnPin, 40) == 0, "replay dependency pins differ");
    require(read(file, 88, 4) == 256 && read(file, 92, 4) == 256, "unsupported framebuffer dimensions");
    bool begun = false, ended = false, drewBatch = false;
    std::size_t memoryBytes = 0;
    for (std::size_t pos = 96; pos < file.size();) {
        require(!ended, "unexpected trailing replay records");
        const auto kind = static_cast<Kind>(read(file, pos, 4));
        const auto size = read(file, pos + 4, 4);
        const auto expected = read(file, pos + 8, 4);
        pos += 12;
        require(size <= file.size() - pos, "truncated replay record");
        Bytes payload(file.begin() + pos, file.begin() + pos + size);
        pos += size;
        require(checksum(payload, kind) == expected, "replay record checksum mismatch");
        if (kind == Kind::Memory) {
            const auto id = static_cast<std::uint32_t>(read(payload, 0, 4));
            require(id > 0 && id <= 64 && payload.size() > 4, "invalid memory record");
            auto& slot = memory[id];
            if (slot.empty()) {
                require(payload.size() - 4 <= MaxBytes - memoryBytes, "replay memory exceeds bound");
                memoryBytes += payload.size() - 4;
                slot.resize(payload.size() - 4);
            }
            require(slot.size() == payload.size() - 4, "memory resource changed size");
            // Keep stable allocations so pointers loaded by a previous batch live.
            std::copy(payload.begin() + 4, payload.end(), slot.begin());
        } else if (kind == Kind::Fifo) {
            require(!payload.empty(), "empty replay FIFO record");
            decoder.relocate(payload, [&](std::uint64_t id, std::size_t needed) -> std::uint64_t {
                const auto it = memory.find(static_cast<std::uint32_t>(id));
                require(id > 0 && id <= 64 && it != memory.end() && needed <= it->second.size(), "unknown or undersized replay resource");
                // Preserve IDs here; host pointer relocation happens during run.
                return id;
            });
            drewBatch = begun;
        } else if (kind == Kind::Begin) {
            require(!begun && !events.empty() && events.back().kind == Kind::Fifo && payload.empty(), "invalid frame begin");
            begun = true;
        } else if (kind == Kind::End) {
            require(begun && drewBatch && !events.empty() && events.back().kind == Kind::Fifo && payload.empty(), "invalid frame end");
            ended = true;
        } else {
            throw std::runtime_error("unknown replay record kind");
        }
        // Validation rewrites IDs to the same IDs, so these bytes stay portable.
        events.push_back({kind, std::move(payload)});
    }
    require(ended, "replay has no completed frame");
}

void Playback::run(const std::function<void(Kind, std::span<const std::uint8_t>)>& consume) const {
    std::map<std::uint32_t, Bytes> live;
    Decoder parser;
    for (const auto& event : events) {
        if (event.kind == Kind::Memory) {
            const auto id = static_cast<std::uint32_t>(read(event.payload, 0, 4));
            auto& data = live[id];
            if (data.empty()) {
                data.resize(event.payload.size() - 4);
            }
            std::copy(event.payload.begin() + 4, event.payload.end(), data.begin());
        } else if (event.kind == Kind::Fifo) {
            auto fifo = event.payload;
            parser.relocate(fifo, [&](std::uint64_t id, std::size_t) {
                return reinterpret_cast<std::uintptr_t>(live.at(static_cast<std::uint32_t>(id)).data());
            });
            consume(event.kind, fifo);
        } else {
            consume(event.kind, {});
        }
    }
}
void set_recorder(Recorder* recorder) {
    active = recorder;
}
} // namespace replay

extern "C" void mkw_replay_capture_drain(const unsigned char* data, unsigned int size) {
    if (replay::active) {
        replay::active->drain({data, size});
    }
}
