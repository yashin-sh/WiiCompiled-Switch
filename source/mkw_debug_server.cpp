#include "mkw_debug_server.hpp"

#include <algorithm>
#include <array>
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#if defined(__SWITCH__)
#include <switch.h>
#else
#include <thread>
#endif

namespace mkw::debug {
namespace {
using Clock = std::chrono::steady_clock;
constexpr std::size_t MaxLine = 511, LogSlots = 64, LogBytes = 512;
constexpr std::uint64_t MaxFile = 512ull * 1024 * 1024;
constexpr char DefaultRoot[] = "sdmc:/switch/WiiCompiled-Switch";
constexpr char DefaultNro[] = "sdmc:/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro";
std::atomic<bool> running{false};
std::atomic<std::uint64_t> dispatch{0}, presents{0}, dropped{0};
std::atomic<std::uint32_t> guest_pc{0};
std::atomic<std::int64_t> next_dispatch_log{0};
std::atomic_flag state_lock = ATOMIC_FLAG_INIT;
struct Record {
    std::uint64_t seq = 0;
    std::size_t size = 0;
    char bytes[LogBytes]{};
};
std::array<Record, LogSlots> records{};
std::uint64_t sequence = 0;
char stage[96] = "startup", frontier[128] = "unknown";
std::string root, nro;
std::uint16_t port = 0;
int listener = -1;
Clock::time_point started;
#if defined(__SWITCH__)
Thread worker_thread{};
bool sockets_ready = false, thread_ready = false;
#else
std::thread worker_thread;
#endif

// Small streaming SHA-256 implementation keeps the host contract dependency-free.
class Sha256 {
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    unsigned char block[64]{};
    std::size_t used = 0;
    std::uint64_t bytes = 0;
    static std::uint32_t rot(std::uint32_t x, unsigned n) {
        return (x >> n) | (x << (32 - n));
    }
    void transform() {
        static constexpr std::uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (std::uint32_t(block[i * 4]) << 24) | (std::uint32_t(block[i * 4 + 1]) << 16) |
                   (std::uint32_t(block[i * 4 + 2]) << 8) | block[i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            auto a = w[i - 15], b = w[i - 2];
            w[i] = w[i - 16] + (rot(a, 7) ^ rot(a, 18) ^ (a >> 3)) +
                   w[i - 7] + (rot(b, 17) ^ rot(b, 19) ^ (b >> 10));
        }
        auto a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], z = h[7];
        for (int i = 0; i < 64; ++i) {
            auto t = z + (rot(e, 6) ^ rot(e, 11) ^ rot(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            auto u = (rot(a, 2) ^ rot(a, 13) ^ rot(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            z = g;
            g = f;
            f = e;
            e = d + t;
            d = c;
            c = b;
            b = a;
            a = t + u;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += z;
    }

  public:
    void add(const void* p, std::size_t n) {
        const auto* input = static_cast<const unsigned char*>(p);
        bytes += n;
        while (n) {
            auto take = std::min(n, 64 - used);
            std::memcpy(block + used, input, take);
            used += take;
            input += take;
            n -= take;
            if (used == 64) {
                transform();
                used = 0;
            }
        }
    }
    std::string finish() {
        auto bits = bytes * 8;
        unsigned char one = 0x80, zero = 0;
        add(&one, 1);
        while (used != 56)
            add(&zero, 1);
        unsigned char length[8];
        for (int i = 0; i < 8; ++i)
            length[7 - i] = static_cast<unsigned char>(bits >> (i * 8));
        add(length, 8);
        char out[65];
        for (int i = 0; i < 8; ++i)
            std::snprintf(out + i * 8, 9, "%08x", h[i]);
        return out;
    }
};
std::uint32_t crc_add(std::uint32_t crc, const unsigned char* p, std::size_t n) {
    static constexpr auto table = [] {
        std::array<std::uint32_t, 256> result{};
        for (std::uint32_t i = 0; i < result.size(); ++i) {
            auto value = i;
            for (unsigned bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ (0xedb88320u & -(value & 1));
            result[i] = value;
        }
        return result;
    }();
    for (std::size_t i = 0; i < n; ++i)
        crc = (crc >> 8) ^ table[(crc ^ p[i]) & 255];
    return crc;
}

bool wait_fd(int fd, bool write) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeval timeout{0, 100000};
    return ::select(fd + 1, write ? nullptr : &set, write ? &set : nullptr, nullptr, &timeout) > 0;
}
bool transfer(int fd, void* data, std::size_t n, bool write) {
    auto* p = static_cast<char*>(data);
    auto last = Clock::now();
    while (n && running.load(std::memory_order_relaxed)) {
        if (!wait_fd(fd, write)) {
            if (Clock::now() - last > std::chrono::seconds(10))
                return false;
            continue;
        }
        const auto chunk = std::min(n, std::size_t(8192));
#ifdef MSG_NOSIGNAL
        constexpr int send_flags = MSG_NOSIGNAL;
#else
        constexpr int send_flags = 0;
#endif
        auto count = write ? ::send(fd, p, chunk, send_flags) : ::recv(fd, p, chunk, 0);
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            continue;
        if (count <= 0)
            return false;
        p += count;
        n -= count;
        last = Clock::now();
    }
    return n == 0;
}
bool send_bytes(int fd, const void* p, std::size_t n) {
    return transfer(fd, const_cast<void*>(p), n, true);
}
bool frame_header(int fd, bool ok, std::uint64_t size) {
    char header[48];
    auto n = std::snprintf(header, sizeof(header), "%s %llu\n", ok ? "ok" : "err", static_cast<unsigned long long>(size));
    return send_bytes(fd, header, n);
}
bool reply(int fd, bool ok, const std::string& body) {
    return frame_header(fd, ok, body.size()) && send_bytes(fd, body.data(), body.size());
}
bool error(int fd, const char* what) {
    return reply(fd, false, std::string(what) + "\n");
}

bool relative_safe(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.find('\\') != std::string::npos || path.find(':') != std::string::npos)
        return false;
    std::size_t start = 0;
    do {
        auto end = path.find('/', start);
        auto part = path.substr(start, end == std::string::npos ? end : end - start);
        if (part.empty() || part == "." || part == "..")
            return false;
        for (unsigned char c : part)
            if (c <= 32 || c > 126)
                return false;
        if (end == std::string::npos)
            break;
        start = end + 1;
    } while (true);
    return true;
}
// Check every existing component, including configured roots, before opening.
bool no_symlinks(const std::string& path, bool allow_missing_leaf = false) {
    if (path.empty())
        return false;
    for (std::size_t i = 0; i <= path.size(); ++i) {
        if (i < path.size() && path[i] != '/')
            continue;
        if (i == 0)
            continue;
        const auto prefix = path.substr(0, i);
        struct stat st{};
        const auto stat_path = prefix.back() == ':' ? prefix + "/" : prefix;
        if (::lstat(stat_path.c_str(), &st) != 0)
            return allow_missing_leaf && i == path.size() && errno == ENOENT;
        if (S_ISLNK(st.st_mode))
            return false;
        if (i < path.size() && !S_ISDIR(st.st_mode))
            return false;
    }
    return true;
}
bool regular(const std::string& path) {
    struct stat st{};
    return no_symlinks(path) && ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
           st.st_size >= 0 && std::uint64_t(st.st_size) <= MaxFile;
}
bool diagnostic(const std::string& rel) {
    if (!relative_safe(rel))
        return false;
    const auto slash = rel.find('/');
    if (slash != std::string::npos) {
        const auto dir = rel.substr(0, slash);
        return dir == "reports" || dir == "Reports" || dir == "logs" || dir == "Logs" || dir == "captures" || dir == "Captures";
    }
    const auto dot = rel.rfind('.');
    if (dot == std::string::npos)
        return false;
    const auto ext = rel.substr(dot);
    return ext == ".txt" || ext == ".log" || ext == ".png" || ext == ".mkwr";
}
std::string directory() {
    return nro.substr(0, nro.rfind('/'));
}
std::string basename() {
    return nro.substr(nro.rfind('/') + 1);
}
bool candidate_name_safe(const std::string& name) {
    return !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
    });
}
bool project_nro(const std::string& name) {
    if (!candidate_name_safe(name))
        return false;
    if (!relative_safe(name) || name.find('/') != std::string::npos)
        return false;
    return name == basename() || name == basename() + ".rollback" || (name.rfind("WiiCompiled-Switch", 0) == 0 && name.size() > 4 && name.substr(name.size() - 4) == ".nro");
}
std::string nro_file(const std::string& name) {
    return project_nro(name) ? directory() + "/" + name : std::string();
}
bool file_hash(const std::string& path, std::uint64_t& size, std::string& sha, std::uint32_t* crc = nullptr) {
    if (!regular(path))
        return false;
    FILE* in = std::fopen(path.c_str(), "rb");
    if (!in)
        return false;
    Sha256 hash;
    std::array<unsigned char, 8192> buf{};
    size = 0;
    std::uint32_t c = ~0u;
    std::size_t n;
    while (running.load() && (n = std::fread(buf.data(), 1, buf.size(), in))) {
        size += n;
        hash.add(buf.data(), n);
        if (crc)
            c = crc_add(c, buf.data(), n);
    }
    bool ok = running.load() && !std::ferror(in);
    std::fclose(in);
    if (ok) {
        sha = hash.finish();
        if (crc)
            *crc = ~c;
    }
    return ok;
}
bool send_file(int fd, const std::string& path) {
    if (!regular(path))
        return error(fd, "file absent, unsafe, or exceeds limit");
    FILE* in = std::fopen(path.c_str(), "rb");
    if (!in)
        return error(fd, "cannot open file");
    struct stat st{};
    if (::fstat(::fileno(in), &st) != 0) {
        std::fclose(in);
        return error(fd, "cannot stat file");
    }
    bool ok = frame_header(fd, true, st.st_size);
    std::array<char, 8192> buf{};
    std::uint64_t left = st.st_size;
    while (ok && left) {
        auto n = std::fread(buf.data(), 1, std::min<std::uint64_t>(left, buf.size()), in);
        if (!n) {
            ok = false;
            break;
        }
        ok = send_bytes(fd, buf.data(), n);
        left -= n;
    }
    std::fclose(in);
    return ok;
}
void list_reports(const std::string& rel, std::string& out, unsigned depth = 0) {
    if (depth > 4 || out.size() > 65536)
        return;
    const auto path = rel.empty() ? root : root + "/" + rel;
    if (!no_symlinks(path))
        return;
    DIR* dir = ::opendir(path.c_str());
    if (!dir)
        return;
    while (auto* entry = ::readdir(dir)) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        auto child = rel.empty() ? name : rel + "/" + name;
        auto full = root + "/" + child;
        if (!no_symlinks(full))
            continue;
        struct stat st{};
        if (::stat(full.c_str(), &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode)) {
            if (rel.empty() && (name != "reports" && name != "Reports" && name != "logs" && name != "Logs" && name != "captures" && name != "Captures"))
                continue;
            list_reports(child, out, depth + 1);
        } else if (S_ISREG(st.st_mode) && diagnostic(child)) {
            out += child;
            out += '\n';
        }
        if (out.size() > 65536)
            break;
    }
    ::closedir(dir);
}
bool can_reload() {
#if defined(__SWITCH__)
    auto type = appletGetAppletType();
    return (type == AppletType_Application || type == AppletType_SystemApplication) && hosversionAtLeast(5, 0, 0);
#else
    return false;
#endif
}
std::string info() {
    char snapshot_stage[sizeof(stage)], snapshot_frontier[sizeof(frontier)];
    if (!state_lock.test_and_set(std::memory_order_acquire)) {
        std::memcpy(snapshot_stage, stage, sizeof(stage));
        std::memcpy(snapshot_frontier, frontier, sizeof(frontier));
        state_lock.clear(std::memory_order_release);
    } else {
        std::strcpy(snapshot_stage, "busy");
        std::strcpy(snapshot_frontier, "busy");
    }
    const auto uptime = std::chrono::duration<double>(Clock::now() - started).count();
    char body[1024];
    std::snprintf(body, sizeof(body),
                  "dispatch=%llu\nguest_pc=0x%08X\nfrontier=%s\nuptime=%.3f\nfps=%.2f\nstage=%s\nlog_dropped=%llu\nreload_supported=%d\ndebug_nro_path=%s\n",
                  static_cast<unsigned long long>(dispatch.load()), guest_pc.load(), snapshot_frontier, uptime,
                  uptime > 0 ? presents.load() / uptime : 0, snapshot_stage, static_cast<unsigned long long>(dropped.load()), can_reload() ? 1 : 0, nro.c_str());
    return body;
}
bool parse_number(const std::string& text, unsigned base, std::uint64_t& number) {
    if (text.empty() || text[0] == '-' || text[0] == '+' || text.size() > 20)
        return false;
    char* end = nullptr;
    errno = 0;
    number = std::strtoull(text.c_str(), &end, base);
    return !errno && end && !*end;
}
bool upload(int fd, const std::string& size_text, const std::string& crc_text) {
    std::uint64_t size = 0, expected_crc = 0;
    if (!parse_number(size_text, 10, size) || size < 0x80 || size > MaxFile || crc_text.size() != 8 ||
        !parse_number(crc_text, 16, expected_crc) || expected_crc > 0xffffffff)
        return error(fd, "invalid upload size or CRC32");
    if (!no_symlinks(nro, true))
        return error(fd, "unsafe deployment path");
    const auto part = nro + ".part";
    const auto rollback = nro + ".rollback";
    struct stat old{};
    if (::lstat(rollback.c_str(), &old) == 0)
        return error(fd, "pending rollback requires verified backup and finalize-deploy");
    if (errno != ENOENT)
        return error(fd, "cannot inspect rollback file");
    if (::lstat(nro.c_str(), &old) == 0 && !regular(nro))
        return error(fd, "unsafe existing candidate");
    if (::lstat(part.c_str(), &old) == 0) {
        if (!S_ISREG(old.st_mode) || ::unlink(part.c_str()) != 0)
            return error(fd, "unsafe or busy partial file");
    } else if (errno != ENOENT)
        return error(fd, "cannot inspect partial file");
    int out = ::open(part.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (out < 0)
        return error(fd, "cannot create partial file");
    bool ok = reply(fd, true, "");
    std::array<unsigned char, 8192> buf{};
    std::array<unsigned char, 128> header{};
    std::uint64_t total = 0;
    std::uint32_t crc = ~0u;
    while (ok && total < size) {
        auto n = std::min<std::uint64_t>(size - total, buf.size());
        ok = transfer(fd, buf.data(), n, false);
        if (!ok)
            break;
        if (total == 0)
            std::memcpy(header.data(), buf.data(), header.size());
        crc = crc_add(crc, buf.data(), n);
        std::size_t written = 0;
        while (written < n) {
            auto count = ::write(out, buf.data() + written, n - written);
            if (count < 0 && errno == EINTR)
                continue;
            if (count <= 0) {
                ok = false;
                break;
            }
            written += count;
        }
        total += n;
    }
    if (ok)
        ok = ::fsync(out) == 0;
    if (::close(out) != 0)
        ok = false;
    if (!ok) {
        ::unlink(part.c_str());
        return false;
    }
    std::uint32_t declared = std::uint32_t(header[24]) | (std::uint32_t(header[25]) << 8) | (std::uint32_t(header[26]) << 16) | (std::uint32_t(header[27]) << 24);
    if (~crc != expected_crc || std::memcmp(header.data() + 16, "NRO0", 4) != 0 || declared < 128 || declared > size) {
        ::unlink(part.c_str());
        return error(fd, "CRC32 or NRO header verification failed");
    }
    std::uint64_t read_size = 0;
    std::uint32_t read_crc = 0;
    std::string sha;
    if (!file_hash(part, read_size, sha, &read_crc) || read_size != size || read_crc != expected_crc) {
        ::unlink(part.c_str());
        return error(fd, "SD readback verification failed");
    }
    // Horizon rename does not replace an existing destination. Preserve the
    // previous candidate until explicit client readback/backup finalization.
    bool had_previous = ::lstat(nro.c_str(), &old) == 0;
    if (had_previous && ::rename(nro.c_str(), rollback.c_str()) != 0) {
        ::unlink(part.c_str());
        return error(fd, "cannot preserve previous candidate");
    }
    if (::rename(part.c_str(), nro.c_str()) != 0) {
        const bool restored = !had_previous || ::rename(rollback.c_str(), nro.c_str()) == 0;
        ::unlink(part.c_str());
        return error(fd, restored ? "cannot commit upload; previous candidate restored" : "cannot commit or restore; previous candidate preserved in rollback");
    }
    char metadata[160];
    std::snprintf(metadata, sizeof(metadata), "size=%llu\ncrc32=%08x\nsha256=%s\n",
                  static_cast<unsigned long long>(size), read_crc, sha.c_str());
    return reply(fd, true, std::string(metadata) + "path=" + nro + "\n");
}
struct Client {
    int fd = -1;
    bool logging = false;
    std::uint64_t cursor = 0;
    std::string line;
    Clock::time_point activity;
};
bool command(Client& client, const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> words;
    std::string word;
    while (input >> word)
        words.push_back(word);
    auto fd = client.fd;
    if (words.empty())
        return error(fd, "empty command");
    const auto& cmd = words[0];
    if (cmd == "ping" && words.size() == 1)
        return reply(fd, true, "pong\n");
    if (cmd == "info" && words.size() == 1)
        return reply(fd, true, info());
    if (cmd == "quit" && words.size() == 1) {
        reply(fd, true, "");
        return false;
    }
    if (cmd == "log" && words.size() == 1) {
        client.logging = true;
        client.cursor = 0;
        return reply(fd, true, "");
    }
    if (cmd == "reports" && words.size() == 1) {
        std::string list;
        list_reports("", list);
        return reply(fd, true, list);
    }
    if (cmd == "get" && words.size() == 2) {
        if (!diagnostic(words[1]))
            return error(fd, "diagnostic path forbidden");
        return send_file(fd, root + "/" + words[1]);
    }
    if (cmd == "list-nros" && words.size() == 1) {
        std::string list;
        if (!no_symlinks(directory()))
            return error(fd, "unsafe candidate directory");
        DIR* dir = ::opendir(directory().c_str());
        if (!dir)
            return error(fd, "cannot list candidate directory");
        while (auto* entry = ::readdir(dir)) {
            std::string name = entry->d_name;
            const bool recognized = name == basename() || name == basename() + ".rollback" ||
                                    (name.rfind("WiiCompiled-Switch", 0) == 0 && name.size() > 4 &&
                                     name.substr(name.size() - 4) == ".nro");
            if (!recognized)
                continue;
            if (!project_nro(name)) {
                ::closedir(dir);
                return error(fd, "unsafe project candidate name prevents cleanup");
            }
            // Inventory must not hide oversized or unsafe old candidates. Their
            // stat/get verification rejects them before the client can deploy.
            list += name + "\n";
        }
        ::closedir(dir);
        return reply(fd, true, list);
    }
    if ((cmd == "get-nro" || cmd == "stat-nro") && (words.size() == 1 || words.size() == 2)) {
        auto path = nro_file(words.size() == 2 ? words[1] : basename());
        if (path.empty())
            return error(fd, "candidate forbidden");
        if (cmd == "get-nro")
            return send_file(fd, path);
        std::uint64_t size;
        std::string sha;
        if (!file_hash(path, size, sha))
            return error(fd, "candidate absent or unsafe");
        return reply(fd, true, "path=" + path + "\nsize=" + std::to_string(size) + "\nsha256=" + sha + "\n");
    }
    if (cmd == "remove-nro" && words.size() == 4) {
        auto path = nro_file(words[1]);
        std::uint64_t expected = 0, actual = 0;
        std::string sha;
        if (path.empty() || path == nro || path == nro + ".rollback" || words[3].size() != 64 || !parse_number(words[2], 10, expected))
            return error(fd, "old candidate forbidden");
        if (!file_hash(path, actual, sha) || actual != expected || sha != words[3])
            return error(fd, "old candidate size/SHA256 mismatch");
        if (::unlink(path.c_str()) != 0)
            return error(fd, "cannot remove old candidate");
        return reply(fd, true, "");
    }
    if (cmd == "finalize-deploy" && words.size() == 5) {
        std::uint64_t expected = 0, actual = 0, previous = 0;
        std::string sha;
        if (words[2].size() != 64 || !parse_number(words[1], 10, expected) ||
            !parse_number(words[3], 10, previous))
            return error(fd, "invalid finalization metadata");
        if (!file_hash(nro, actual, sha) || actual != expected || sha != words[2])
            return error(fd, "current candidate size/SHA256 mismatch");
        const auto rollback = nro + ".rollback";
        struct stat st{};
        if (::lstat(rollback.c_str(), &st) != 0) {
            if (errno == ENOENT && previous == 0 && words[4] == "-")
                return reply(fd, true, "");
            return error(fd, "rollback absent or unreadable");
        }
        if (words[4].size() != 64 || !file_hash(rollback, actual, sha) || actual != previous || sha != words[4])
            return error(fd, "rollback backup size/SHA256 mismatch");
        if (::unlink(rollback.c_str()) != 0)
            return error(fd, "cannot finalize rollback removal");
        return reply(fd, true, "");
    }
    if (cmd == "deploy" && words.size() == 3)
        return upload(fd, words[1], words[2]);
    if (cmd == "reload" && words.size() == 1) {
        if (!can_reload())
            return error(fd, "reload requires a HOME forwarder application on system 5.0.0 or newer");
#if defined(__SWITCH__)
        if (!reply(fd, true, "restart requested\n"))
            return false;
        const Result result = appletRestartProgram(nullptr, 0);
        if (R_FAILED(result)) {
            char report[96];
            std::snprintf(report, sizeof(report), "restart failed: 0x%08x\n", result);
            log(report, std::strlen(report));
            reply(fd, false, report);
        }
        return false;
#endif
    }
    return error(fd, "unknown command or argument count");
}
void close_client(Client& c) {
    if (c.fd >= 0)
        ::close(c.fd);
    c = Client{};
}
bool flush_log(Client& c) {
    Record record;
    bool available = false;
    std::uint64_t lost = 0;
    if (state_lock.test_and_set(std::memory_order_acquire))
        return true;
    const auto oldest = sequence >= LogSlots ? sequence - LogSlots + 1 : 1;
    if (c.cursor && c.cursor + 1 < oldest)
        lost = oldest - (c.cursor + 1);
    auto next = std::max(c.cursor + 1, oldest);
    if (next <= sequence) {
        record = records[(next - 1) % LogSlots];
        available = true;
        c.cursor = next;
    }
    state_lock.clear(std::memory_order_release);
    if (lost && !reply(c.fd, true, "[debug log dropped " + std::to_string(lost) + " records]\n"))
        return false;
    return !available || (frame_header(c.fd, true, record.size) && send_bytes(c.fd, record.bytes, record.size));
}
void worker(void*) noexcept {
    std::array<Client, 4> clients{};
    try {
        while (running.load()) {
            fd_set read_set;
            FD_ZERO(&read_set);
            FD_SET(listener, &read_set);
            int highest = listener;
            for (auto& c : clients)
                if (c.fd >= 0) {
                    FD_SET(c.fd, &read_set);
                    highest = std::max(highest, c.fd);
                }
            timeval timeout{0, 100000};
            int ready = ::select(highest + 1, &read_set, nullptr, nullptr, &timeout);
            if (ready < 0 && errno != EINTR)
                break;
            if (ready > 0 && FD_ISSET(listener, &read_set)) {
                int fd = ::accept(listener, nullptr, nullptr);
                if (fd >= FD_SETSIZE) {
                    ::close(fd);
                    fd = -1;
                }
                if (fd >= 0) {
                    ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
                    auto slot = std::find_if(clients.begin(), clients.end(), [](const Client& c) { return c.fd < 0; });
                    if (slot == clients.end()) {
                        error(fd, "client limit reached");
                        ::close(fd);
                    } else {
                        slot->fd = fd;
                        slot->activity = Clock::now();
                    }
                }
            }
            for (auto& c : clients) {
                if (c.fd < 0)
                    continue;
                if (ready > 0 && FD_ISSET(c.fd, &read_set)) {
                    // Read exactly through the command newline; binary upload bytes stay in the socket.
                    char byte;
                    auto n = ::recv(c.fd, &byte, 1, 0);
                    if (n == 0) {
                        close_client(c);
                        continue;
                    }
                    if (n < 0) {
                        if (errno != EAGAIN && errno != EINTR)
                            close_client(c);
                        continue;
                    }
                    c.activity = Clock::now();
                    if (c.logging) {
                        close_client(c);
                        continue;
                    }
                    if (byte == '\n') {
                        auto line = c.line;
                        c.line.clear();
                        if (!command(c, line)) {
                            close_client(c);
                            continue;
                        }
                    } else if (byte == '\r') {
                    } else if (static_cast<unsigned char>(byte) < 32 || static_cast<unsigned char>(byte) > 126 || c.line.size() >= MaxLine) {
                        error(c.fd, "invalid or oversized command");
                        close_client(c);
                        continue;
                    } else
                        c.line += byte;
                }
                if (c.fd >= 0 && c.logging && !flush_log(c)) {
                    close_client(c);
                    continue;
                }
                if (c.fd >= 0 && !c.logging && !c.line.empty() && Clock::now() - c.activity > std::chrono::seconds(10))
                    close_client(c);
            }
        }
    } catch (...) { /* Keep faults confined to diagnostics; stop still joins. */
    }
    running.store(false);
    for (auto& c : clients)
        close_client(c);
}
bool launch(bool opt_in, const std::string& app_root, const std::string& candidate, std::uint16_t requested_port) {
    if (running.load())
        return true;
    if (!opt_in)
        return false;
    if (listener >= 0)
        stop();
    if (app_root.empty() || candidate.empty() || candidate.rfind('/') == std::string::npos || candidate.size() > 384)
        return false;
#if defined(__SWITCH__)
    if (candidate.rfind("sdmc:/switch/", 0) != 0)
        return false;
#endif
    auto rel = candidate.substr(candidate.find(':') == std::string::npos ? (candidate.front() == '/' ? 1 : 0) : candidate.find(':') + 2);
    if (!candidate_name_safe(candidate.substr(candidate.rfind('/') + 1)) ||
        !relative_safe(rel) || candidate.size() < 4 || candidate.substr(candidate.size() - 4) != ".nro" || !no_symlinks(app_root) || !no_symlinks(candidate, true))
        return false;
    root = app_root;
    nro = candidate;
    // Recover a process/power interruption between the two commit renames.
    const auto rollback = nro + ".rollback";
    struct stat current{};
    if (::lstat(nro.c_str(), &current) != 0) {
        if (errno != ENOENT)
            return false;
        struct stat previous{};
        if (::lstat(rollback.c_str(), &previous) == 0) {
            if (!regular(rollback) || ::rename(rollback.c_str(), nro.c_str()) != 0)
                return false;
        } else if (errno != ENOENT)
            return false;
    }
#if defined(__SWITCH__)
    if (R_FAILED(socketInitializeDefault()))
        return false;
    sockets_ready = true;
#endif
    listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0 || listener >= FD_SETSIZE) {
        stop();
        return false;
    }
    int yes = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(requested_port);
    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || ::listen(listener, 4) != 0) {
        stop();
        return false;
    }
    socklen_t length = sizeof(address);
    if (::getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        stop();
        return false;
    }
    port = ntohs(address.sin_port);
    ::fcntl(listener, F_SETFL, ::fcntl(listener, F_GETFL, 0) | O_NONBLOCK);
    started = Clock::now();
    running = true;
#if defined(__SWITCH__)
    if (R_FAILED(threadCreate(&worker_thread, worker, nullptr, nullptr, 0x10000, 0x3b, -2))) {
        stop();
        return false;
    }
    thread_ready = true;
    if (R_FAILED(threadStart(&worker_thread))) {
        running = false;
        threadClose(&worker_thread);
        thread_ready = false;
        stop();
        return false;
    }
#else
    worker_thread = std::thread([] { worker(nullptr); });
#endif
    return true;
}
} // namespace

bool start() noexcept {
    try {
        std::string app_root = DefaultRoot, candidate = DefaultNro;
        bool opt_in = false;
        std::uint16_t requested_port = 6543;
        FILE* config = std::fopen((app_root + "/Config/debug.ini").c_str(), "r");
        if (!config)
            return false;
        char line[512];
        while (std::fgets(line, sizeof(line), config)) {
            std::string text = line;
            auto comment = text.find_first_of("#;");
            if (comment != std::string::npos)
                text.resize(comment);
            auto equals = text.find('=');
            if (equals == std::string::npos)
                continue;
            auto trim = [](std::string value) {auto a=value.find_first_not_of(" \t\r\n");if(a==std::string::npos)return std::string();return value.substr(a,value.find_last_not_of(" \t\r\n")-a+1); };
            auto key = trim(text.substr(0, equals)), value = trim(text.substr(equals + 1));
            if (key == "debug_server")
                opt_in = value == "1";
            else if (key == "debug_nro_path")
                candidate = value;
            else if (key == "debug_port") {
                std::uint64_t number;
                if (!parse_number(value, 10, number) || number == 0 || number > 65535) {
                    std::fclose(config);
                    return false;
                }
                requested_port = number;
            }
        }
        std::fclose(config);
        return launch(opt_in, app_root, candidate, requested_port);
    } catch (...) {
        stop();
        return false;
    }
}
void stop() noexcept {
    running.store(false);
#if defined(__SWITCH__)
    if (thread_ready) {
        threadWaitForExit(&worker_thread);
        threadClose(&worker_thread);
        thread_ready = false;
    }
#else
    if (worker_thread.joinable())
        worker_thread.join();
#endif
    if (listener >= 0) {
        ::close(listener);
        listener = -1;
    }
    port = 0;
#if defined(__SWITCH__)
    if (sockets_ready) {
        socketExit();
        sockets_ready = false;
    }
#endif
}
bool enabled() noexcept {
    return running.load(std::memory_order_relaxed);
}
void note_stage(const char* value) noexcept {
    if (!enabled() || !value || state_lock.test_and_set(std::memory_order_acquire))
        return;
    bool changed = std::strncmp(stage, value, sizeof(stage) - 1) != 0;
    char snapshot_frontier[sizeof(frontier)];
    std::memcpy(snapshot_frontier, frontier, sizeof(frontier));
    std::snprintf(stage, sizeof(stage), "%s", value);
    state_lock.clear(std::memory_order_release);
    if (changed) {
        char event[384];
        auto length = std::snprintf(event, sizeof(event), "stage=%.*s dispatch=%llu guest_pc=0x%08X frontier=%s\n",
                                    95, value, static_cast<unsigned long long>(dispatch.load()), guest_pc.load(), snapshot_frontier);
        log(event, std::min<std::size_t>(length, sizeof(event) - 1));
    }
}
void note_dispatch(std::uint64_t count, std::uint32_t pc, const char* value) noexcept {
    if (!enabled())
        return;
    dispatch.store(count, std::memory_order_relaxed);
    guest_pc.store(pc, std::memory_order_relaxed);
    if (value && !state_lock.test_and_set(std::memory_order_acquire)) {
        std::snprintf(frontier, sizeof(frontier), "%s", value);
        state_lock.clear(std::memory_order_release);
    }
    // Sampling is opt-in and bounded; no producer ever waits for the worker.
    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
    auto next = next_dispatch_log.load(std::memory_order_relaxed);
    if (now >= next && next_dispatch_log.compare_exchange_strong(next, now + 1000000000, std::memory_order_relaxed)) {
        char event[256];
        auto length = std::snprintf(event, sizeof(event), "dispatch=%llu guest_pc=0x%08X frontier=%.*s\n",
                                    static_cast<unsigned long long>(count), pc, 127, value ? value : "unknown");
        log(event, std::min<std::size_t>(length, sizeof(event) - 1));
    }
}
void note_present() noexcept {
    if (enabled())
        presents.fetch_add(1, std::memory_order_relaxed);
}
void log(const char* bytes, std::size_t size) noexcept {
    if (!enabled() || !bytes || !size)
        return;
    if (state_lock.test_and_set(std::memory_order_acquire)) {
        dropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    // Split existing diagnostic buffers into fixed records, capped at one ring.
    const auto accepted = std::min(size, LogBytes * LogSlots);
    std::size_t offset = 0;
    while (offset < accepted) {
        auto& record = records[sequence % LogSlots];
        record.seq = ++sequence;
        record.size = std::min(accepted - offset, LogBytes);
        std::memcpy(record.bytes, bytes + offset, record.size);
        offset += record.size;
    }
    if (size > accepted)
        dropped.fetch_add(1, std::memory_order_relaxed);
    state_lock.clear(std::memory_order_release);
}
#if !defined(__SWITCH__)
bool start(const HostConfig& config) noexcept {
    try {
        return launch(config.enabled, config.root ? config.root : "", config.nro_path ? config.nro_path : "", config.port);
    } catch (...) {
        stop();
        return false;
    }
}
std::uint16_t bound_port() noexcept {
    return port;
}
#endif
} // namespace mkw::debug
