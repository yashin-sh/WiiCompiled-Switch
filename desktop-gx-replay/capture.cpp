#include "capture.hpp"

#include <algorithm>
#include <cstring>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace replay {
namespace {
Recorder* active = nullptr;
char captureFailure[256]{};
void (*failureHandler)(const char*) noexcept = nullptr;
constexpr std::array<std::uint8_t, 8> Magic{'M', 'K', 'W', 'R', 'P', 'L', '2', 0};
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
                vat[0][reg - 0x70] = value;
            } else if (reg >= 0x80 && reg <= 0x87) {
                vat[1][reg - 0x80] = value;
            } else if (reg >= 0x90 && reg <= 0x97) {
                vat[2][reg - 0x90] = value;
            } else if (reg >= 0xB0 && reg <= 0xBF) {
                arrayStride[reg - 0xB0] = value & 255;
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
                arraySize[sub - 0x10] = needed;
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
            } else if (sub == 0x34) {
                length = 11;
                pointer = pos + 3;
                needed = 1;
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
        } else if (opcode == 0x20 || opcode == 0x28 || opcode == 0x30 || opcode == 0x38) {
            length = 5;
            const auto value = read(fifo, pos + 1, 4);
            const auto slot = 12 + (opcode - 0x20) / 8;
            const auto offset = (value >> 16) * arrayStride[slot];
            const auto size = (((value >> 12) & 15) + 1) * 4;
            require(arrayStride[slot] && offset <= arraySize[slot] && size <= arraySize[slot] - offset, "invalid indexed XF resource");
        } else if (opcode >= 0x80 && opcode <= 0xBF) {
            const auto primitive = opcode & 0xF8;
            const auto count = read(fifo, pos + 1, 2);
            require(count > 0 && ((primitive != 0x80 && primitive != 0x88) || count % 4 == 0) && (primitive != 0x90 || count % 3 == 0) && (primitive != 0xA8 || count % 2 == 0) && (primitive != 0xB0 || count >= 2) && ((primitive != 0x98 && primitive != 0xA0) || count >= 3), "invalid vertex count");
            const auto a = vat[0][opcode & 7], b = vat[1][opcode & 7], c = vat[2][opcode & 7];
            const auto scalar = [&](unsigned type) -> unsigned {
                require(type <= 4, "invalid scalar vertex format");
                return type < 2 ? 1 : type < 4 ? 2
                                               : 4;
            };
            struct Attribute {
                unsigned mode, bytes, indices, slot;
            };
            std::vector<Attribute> attributes;
            for (unsigned i = 0; i < 9; ++i)
                if ((vcdLo >> i) & 1)
                    attributes.push_back({1, 1, 1, 0});
            require(((vcdLo >> 9) & 3) != 0, "missing vertex position");
            attributes.push_back({(vcdLo >> 9) & 3, scalar((a >> 1) & 7) * (2 + (a & 1)), 1, 0});
            const bool nbt = (a >> 9) & 1, nbt3 = nbt && (a >> 31);
            attributes.push_back({(vcdLo >> 11) & 3, scalar((a >> 10) & 7) * (nbt && !nbt3 ? 9u : 3u), nbt3 ? 3u : 1u, 1});
            constexpr unsigned colorBytes[]{2, 3, 4, 2, 3, 4};
            for (unsigned i = 0; i < 2; ++i) {
                const auto mode = (vcdLo >> (13 + i * 2)) & 3, type = (a >> (14 + i * 4)) & 7;
                require(!mode || type < 6, "invalid color vertex format");
                attributes.push_back({mode, mode ? colorBytes[type] : 0, 1, 2 + i});
            }
            for (unsigned i = 0; i < 8; ++i) {
                const auto mode = (vcdHi >> (i * 2)) & 3;
                const unsigned word = i == 0 ? a : i < 5 ? b
                                                         : c;
                const unsigned shift = i == 0 ? 21 : i < 5 ? (i - 1) * 9
                                                           : 5 + (i - 5) * 9;
                const auto type = (word >> (shift + 1)) & 7;
                attributes.push_back({mode, mode ? scalar(type) * (1 + ((word >> shift) & 1)) : 0, 1, 4 + i});
            }
            auto cursor = pos + 3;
            for (unsigned vertex = 0; vertex < count; ++vertex) {
                for (const auto& attr : attributes) {
                    if (attr.mode == 1) {
                        // NBT3 means three separate indices; direct NBT stays 9 scalars.
                        const auto size = attr.bytes * attr.indices;
                        require(cursor <= fifo.size() && size <= fifo.size() - cursor, "truncated direct vertex");
                        cursor += size;
                    } else if (attr.mode >= 2) {
                        require(arraySize[attr.slot] && arrayStride[attr.slot], "indexed vertex has no resource");
                        for (unsigned i = 0; i < attr.indices; ++i) {
                            const auto index = read(fifo, cursor, attr.mode - 1);
                            cursor += attr.mode - 1;
                            const auto offset = index * arrayStride[attr.slot];
                            require(offset <= arraySize[attr.slot] && attr.bytes <= arraySize[attr.slot] - offset, "indexed vertex outside resource");
                        }
                    }
                }
            }
            length = cursor - pos;
        } else {
            throw std::runtime_error("unsupported FIFO opcode (including nested lists)");
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
    const auto base = reinterpret_cast<std::uintptr_t>(data.data());
    require(base <= UINTPTR_MAX - data.size(), "invalid resource address");
    for (const auto& range : ranges) {
        const auto other = reinterpret_cast<std::uintptr_t>(range.data);
        require(base + data.size() <= other || other + range.size <= base, "overlapping capture resources");
    }
    ranges.push_back({data.data(), data.size()});
}
Recorder::Recorder(std::uint32_t w, std::uint32_t h) {
    require(w > 0 && w <= 1920 && h > 0 && h <= 1080, "invalid capture dimensions");
    width = w;
    height = h;
}
void Recorder::resolve_with(std::function<std::span<const std::uint8_t>(std::uint64_t, std::size_t)> fn) {
    resolver = std::move(fn);
}
std::uint64_t Recorder::resource(std::uint64_t address, std::size_t needed) {
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        if (address == reinterpret_cast<std::uintptr_t>(ranges[i].data)) {
            require(needed <= ranges[i].size, "resource extent grew during capture");
            return i + 1;
        }
    }
    require(static_cast<bool>(resolver), "FIFO references unregistered memory");
    auto data = resolver(address, needed);
    require(reinterpret_cast<std::uintptr_t>(data.data()) == address && data.size() == needed, "unmapped capture resource");
    memory(data);
    return ranges.size();
}
void Recorder::snapshot() {
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        require(total <= MaxBytes - 16 && ranges[i].size <= MaxBytes - total - 16, "capture memory exceeds bound");
        Bytes payload;
        integer(payload, i + 1);
        payload.insert(payload.end(), ranges[i].data, ranges[i].data + ranges[i].size);
        append(Kind::Memory, std::move(payload));
    }
}
void Recorder::init() {
    require(!initialized && !ended, "repeated GXInit is outside the initialized capture prefix");
    append(Kind::Init, {});
    initialized = true;
    decoder = {};
}
void Recorder::mapping(std::uint32_t policy) {
    require(policy <= 2, "invalid viewport policy");
    Bytes payload;
    integer(payload, policy);
    append(Kind::Mapping, std::move(payload));
}
void Recorder::copy(Kind kind, std::uint64_t destination, const CopyState& state) {
    require(frameOpen && initialized, "copy outside initialized frame");
    Bytes payload(8);
    for (auto word : state)
        integer(payload, word);
    validate_copy(kind, payload);
    if (kind == Kind::CopyTex) {
        write(payload, 0, resource(destination, texture_size(state[5], state[6], state[7])), 8);
        snapshot();
    }
    append(kind, std::move(payload));
}
void Recorder::raw_draw(unsigned primitive, unsigned format, std::span<const std::uint8_t> vertices, std::uint16_t count) {
    require(primitive >= 0x80 && primitive <= 0xB8 && (primitive & 7) == 0 && format < 8 && count > 0 && !vertices.empty() && vertices.size() <= MaxBytes - 3, "invalid direct raw draw");
    Bytes fifo{static_cast<std::uint8_t>(primitive | format), static_cast<std::uint8_t>(count >> 8), static_cast<std::uint8_t>(count)};
    fifo.insert(fifo.end(), vertices.begin(), vertices.end());
    drain(fifo);
}
void Recorder::drain(std::span<const std::uint8_t> data) {
    require(initialized && !ended && !data.empty() && data.size() <= MaxBytes, "invalid capture drain");
    Bytes fifo(data.begin(), data.end());
    decoder.relocate(fifo, [&](std::uint64_t address, std::size_t needed) { return resource(address, needed); });
    // Snapshot live bytes at consumption, including earlier loaded slots.
    snapshot();
    append(Kind::Fifo, std::move(fifo));
}
void Recorder::begin() {
    require(!frameOpen && !ended, "capture frame already open or ended");
    append(Kind::Begin, {});
    frameOpen = true;
}
void Recorder::end() {
    require(frameOpen && initialized && !events.empty() && (events.back().kind == Kind::Fifo || events.back().kind == Kind::CopyDisp || events.back().kind == Kind::CopyTex), "frame must end after rendering work");
    append(Kind::End, {});
    ended = true;
    frameOpen = false;
}
void Recorder::frame() {
    require(frameOpen && initialized && !events.empty() &&
                (events.back().kind == Kind::Fifo || events.back().kind == Kind::CopyDisp || events.back().kind == Kind::CopyTex),
            "frame must complete after rendering work");
    append(Kind::Frame, {});
    sequence = true;
    frameOpen = false;
    completedEvents = events.size();
}
Bytes Recorder::checkpoint() const {
    require(completedEvents != 0, "no completed frame checkpoint");
    return encode(completedEvents, true);
}
Bytes Recorder::finish() const {
    require(ended, "incomplete capture cannot be saved");
    return encode(events.size(), false);
}
Bytes Recorder::encode(std::size_t count, bool closeLastFrame) const {
    Bytes result(Magic.begin(), Magic.end());
    if (sequence)
        result[6] = '3';
    result.insert(result.end(), WiiPin, WiiPin + 40);
    result.insert(result.end(), DawnPin, DawnPin + 40);
    integer(result, width);
    integer(result, height);
    integer(result, checksum(result, Kind::Init));
    for (std::size_t i = 0; i < count; ++i) {
        const auto& event = events[i];
        const auto kind = closeLastFrame && i + 1u == count ? Kind::End : event.kind;
        integer(result, static_cast<std::uint32_t>(kind));
        integer(result, event.payload.size());
        integer(result, checksum(event.payload, kind));
        result.insert(result.end(), event.payload.begin(), event.payload.end());
    }
    return result;
}

Playback::Playback(std::span<const std::uint8_t> file) {
    require(file.size() >= 100 && file.size() <= MaxBytes, "invalid replay size");
    const bool sequence = file[6] == '3';
    auto magic = Magic;
    if (sequence)
        magic[6] = '3';
    require(std::equal(magic.begin(), magic.end(), file.begin()), "invalid replay magic/version");
    require(std::memcmp(file.data() + 8, WiiPin, 40) == 0 && std::memcmp(file.data() + 48, DawnPin, 40) == 0, "replay dependency pins differ");
    require(read(file, 96, 4) == checksum(file.first(96), Kind::Init), "replay header checksum mismatch");
    width = read(file, 88, 4);
    height = read(file, 92, 4);
    require(width > 0 && width <= 1920 && height > 0 && height <= 1080, "unsupported framebuffer dimensions");
    bool begun = false, ended = false, drewBatch = false, initialized = false, sawFrame = false;
    std::size_t memoryBytes = 0;
    for (std::size_t pos = 100; pos < file.size();) {
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
            require(initialized && !payload.empty(), "FIFO before initialization or empty FIFO");
            decoder.relocate(payload, [&](std::uint64_t id, std::size_t needed) -> std::uint64_t {
                const auto it = memory.find(static_cast<std::uint32_t>(id));
                require(id > 0 && id <= 64 && it != memory.end() && needed <= it->second.size(), "unknown or undersized replay resource");
                // Preserve IDs here; host pointer relocation happens during run.
                return id;
            });
            drewBatch = begun;
        } else if (kind == Kind::Begin) {
            require(!begun && (!sawFrame || sequence) && payload.empty(), "invalid frame begin");
            begun = sawFrame = true;
            drewBatch = false;
        } else if (kind == Kind::End || kind == Kind::Frame) {
            require(begun && drewBatch && !events.empty() && (events.back().kind == Kind::Fifo || events.back().kind == Kind::CopyDisp || events.back().kind == Kind::CopyTex) && payload.empty(), "invalid frame end");
            require(kind != Kind::Frame || sequence, "frame boundary requires replay v3");
            ended = kind == Kind::End;
            begun = false;
            drewBatch = false;
        } else if (kind == Kind::Init) {
            require(!initialized && payload.empty(), "invalid GXInit event");
            initialized = true;
            decoder = {};
        } else if (kind == Kind::Mapping) {
            require(payload.size() == 4 && read(payload, 0, 4) <= 2, "invalid viewport mapping");
        } else if (kind == Kind::CopyDisp || kind == Kind::CopyTex) {
            require(begun && initialized, "copy outside initialized frame");
            validate_copy(kind, payload);
            const auto id = read(payload, 0, 8);
            if (kind == Kind::CopyTex) {
                const auto needed = texture_size(read(payload, 28, 4), read(payload, 32, 4), read(payload, 36, 4));
                require(id > 0 && id <= 64 && memory.contains(id) && needed <= memory.at(id).size(), "unknown copy destination");
            } else {
                require(id == 0, "display copy must not contain pointer");
            }
            drewBatch = true;
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
        } else if (event.kind == Kind::CopyTex) {
            auto payload = event.payload;
            write(payload, 0, reinterpret_cast<std::uintptr_t>(live.at(read(payload, 0, 8)).data()), 8);
            consume(event.kind, payload);
        } else {
            consume(event.kind, event.payload);
        }
    }
}
void set_failure_handler(void (*handler)(const char*) noexcept) {
    failureHandler = handler;
}
void set_recorder(Recorder* recorder) {
    active = recorder;
    if (recorder)
        captureFailure[0] = 0;
}
bool recording() noexcept {
    return active != nullptr;
}
const char* failure() noexcept {
    return captureFailure;
}
void fail(const char* reason) noexcept {
    if (active) {
        std::snprintf(captureFailure, sizeof(captureFailure), "%s", reason);
        active = nullptr;
        if (failureHandler)
            failureHandler(captureFailure);
    }
}
template <class F>
void observe(F&& fn) noexcept {
    if (!active)
        return;
    try {
        fn(*active);
    } catch (const std::exception& error) {
        fail(error.what());
    } catch (...) {
        fail("capture allocation/unknown failure");
    }
}
void capture_mapping(std::uint32_t policy) noexcept {
    observe([&](Recorder& recorder) { recorder.mapping(policy); });
}
void capture_copy(Kind kind, std::uint64_t destination, const CopyState& state) noexcept {
    observe([&](Recorder& recorder) { recorder.copy(kind, destination, state); });
}
void validate_copy(Kind kind, std::span<const std::uint8_t> payload) {
    require((kind == Kind::CopyDisp || kind == Kind::CopyTex) && payload.size() == 8 + CopyWords * 4, "invalid copy event");
    const auto word = [&](unsigned i) { return static_cast<std::uint32_t>(read(payload, 8 + i * 4, 4)); };
    for (auto i : {0, 8, 9, 15, 16, 55, 56, 57})
        require(word(i) <= 1, "invalid copy boolean");
    for (auto i : {1, 2, 3, 4, 5, 6})
        require(word(i) <= 1920, "invalid copy rectangle");
    require(word(3) > 0 && word(4) > 0 && word(5) > 0 && word(6) > 0, "empty copy rectangle");
    require(word(10) <= 2 && word(11) <= 3 && word(12) <= 3 && word(13) <= 3 && word(53) <= 7 && word(54) <= 3, "invalid copy enum");
    const auto scale = std::bit_cast<float>(word(14));
    require(std::isfinite(scale) && scale > 0 && scale <= 4, "invalid copy scale");
    for (unsigned i = 17; i < 41; ++i)
        require(word(i) <= 15, "invalid copy sample");
    for (unsigned i = 41; i < 48; ++i)
        require(word(i) <= 63, "invalid copy filter");
    for (unsigned i = 48; i < 52; ++i) {
        const auto value = std::bit_cast<float>(word(i));
        require(std::isfinite(value) && value >= 0 && value <= 1, "invalid clear color");
    }
    require(word(52) <= 0xffffff && (word(58) <= 255 || word(58) == UINT32_MAX), "invalid copy clear state");
    if (kind == Kind::CopyTex)
        texture_size(word(5), word(6), word(7));
    else
        require(word(7) == 6 && word(8) == 0 && word(9) == 0, "invalid display copy configuration");
}
} // namespace replay

extern "C" void mkw_replay_capture_drain(const unsigned char* data, unsigned int size) noexcept {
    replay::observe([&](replay::Recorder& recorder) { recorder.drain({data, size}); });
}
extern "C" void mkw_replay_capture_init() noexcept {
    replay::observe([](replay::Recorder& recorder) { recorder.init(); });
}
extern "C" void mkw_replay_capture_unsupported(const char* reason) noexcept {
    replay::fail(reason);
}

extern "C" void mkw_replay_capture_raw_draw(unsigned primitive, unsigned format, const unsigned char* vertices, unsigned short count, unsigned size) noexcept {
    replay::observe([&](replay::Recorder& recorder) { recorder.raw_draw(primitive, format, {vertices, size}, count); });
}
