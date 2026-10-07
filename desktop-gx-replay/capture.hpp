#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <vector>

namespace replay {
using Bytes = std::vector<std::uint8_t>;
constexpr std::size_t MaxBytes = 8 * 1024 * 1024;
constexpr const char* WiiPin = "a135beb201042b20f390c6695ca6b26768820fb4";
constexpr const char* DawnPin = "77029ea85250c9bdddfc2f88034afb6b5356a031";
enum class Kind : std::uint32_t { Memory = 1,
                                  Fifo = 2,
                                  Begin = 3,
                                  End = 4 };
struct Event {
    Kind kind;
    Bytes payload;
};

// Deliberately bounded subset: direct XYZ/F32, optional RGBA8 and ST/F32.
// No indirect XF, guest CP array bases, nested lists or EFB copy commands.
class Decoder {
    std::uint32_t vcdLo = 0;
    std::uint32_t vcdHi = 0;
    std::array<std::uint32_t, 8> vat{};

  public:
    using Relocate = std::function<std::uint64_t(std::uint64_t, std::size_t)>;
    void relocate(Bytes& fifo, const Relocate& resolve);
};

class Recorder {
    struct Range {
        const std::uint8_t* data;
        std::size_t size;
    };
    std::vector<Range> ranges;
    std::vector<Event> events;
    Decoder decoder;
    std::size_t total = 96;
    bool begun = false;
    bool ended = false;
    void append(Kind kind, Bytes payload);

  public:
    void memory(std::span<const std::uint8_t> data);
    void drain(std::span<const std::uint8_t> data);
    void begin();
    void end();
    Bytes finish() const;
};

class Playback {
    std::vector<Event> events;
    std::map<std::uint32_t, Bytes> memory;
    Decoder decoder;

  public:
    explicit Playback(std::span<const std::uint8_t> file);
    // Validation and pointer relocation finish before any GPU callback runs.
    // Consume each batch synchronously; resource pointers live until run returns.
    void run(const std::function<void(Kind, std::span<const std::uint8_t>)>& consume) const;
};
void set_recorder(Recorder* recorder);
} // namespace replay
