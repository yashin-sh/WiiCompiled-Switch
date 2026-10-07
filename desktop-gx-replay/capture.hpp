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
                                  End = 4,
                                  Init = 5,
                                  CopyDisp = 6,
                                  CopyTex = 7,
                                  Mapping = 8 };
struct Event {
    Kind kind;
    Bytes payload;
};

// GX scalar, color, matrix-index and indexed attributes. Guest CP bases,
// nested list commands and raw BP EFB-copy triggers remain unsupported.
class Decoder {
    std::uint32_t vcdLo = 0;
    std::uint32_t vcdHi = 0;
    std::array<std::array<std::uint32_t, 8>, 3> vat{};
    std::array<std::uint32_t, 16> arraySize{}, arrayStride{};

  public:
    using Relocate = std::function<std::uint64_t(std::uint64_t, std::size_t)>;
    void relocate(Bytes& fifo, const Relocate& resolve);
};

constexpr std::size_t CopyWords = 59;
using CopyState = std::array<std::uint32_t, CopyWords>;
void validate_copy(Kind kind, std::span<const std::uint8_t> payload);
void apply_direct(Kind kind, std::span<const std::uint8_t> payload);

class Recorder {
    struct Range {
        const std::uint8_t* data;
        std::size_t size;
    };
    std::vector<Range> ranges;
    std::vector<Event> events;
    Decoder decoder;
    std::function<std::span<const std::uint8_t>(std::uint64_t, std::size_t)> resolver;
    std::uint32_t width, height;
    bool initialized = false;
    std::uint64_t resource(std::uint64_t address, std::size_t needed);
    void snapshot();
    std::size_t total = 100;
    bool begun = false;
    bool ended = false;
    void append(Kind kind, Bytes payload);

  public:
    explicit Recorder(std::uint32_t width = 256, std::uint32_t height = 256);
    void resolve_with(std::function<std::span<const std::uint8_t>(std::uint64_t, std::size_t)> fn);
    void init();
    void mapping(std::uint32_t policy);
    void copy(Kind kind, std::uint64_t destination, const CopyState& state);
    void memory(std::span<const std::uint8_t> data);
    void raw_draw(unsigned primitive, unsigned format, std::span<const std::uint8_t> vertices, std::uint16_t count);
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
    std::uint32_t width = 0, height = 0;
    explicit Playback(std::span<const std::uint8_t> file);
    // Validation and pointer relocation finish before any GPU callback runs.
    // Consume each batch synchronously; resource pointers live until run returns.
    void run(const std::function<void(Kind, std::span<const std::uint8_t>)>& consume) const;
};
void set_failure_handler(void (*handler)(const char*) noexcept);
void set_recorder(Recorder* recorder);
bool recording() noexcept;
const char* failure() noexcept;
void fail(const char* reason) noexcept;
// Hooks are observational: failure disables capture and never escapes into GX.
void capture_mapping(std::uint32_t policy) noexcept;
void capture_copy(Kind kind, std::uint64_t destination, const CopyState& state) noexcept;

} // namespace replay
