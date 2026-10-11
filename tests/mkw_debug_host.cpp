#include "mkw_debug_server.hpp"

#include <atomic>
#include <cerrno>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
std::atomic<int> rename_fault{0};
}
extern "C" int __real_rename(const char*, const char*);
extern "C" int __wrap_rename(const char* source, const char* destination) {
    const std::string path(source);
    if (path.ends_with(".part")) {
        const auto fault = rename_fault.exchange(0);
        if (fault == 1) {
            errno = EIO;
            return -1;
        }
        if (fault == 2)
            ::_exit(90); // Power-loss seam: after rotation, before candidate commit.
    }
    // Horizon fsFsRenameFile rejects an existing destination. POSIX rename
    // ordinarily replaces it, which hid a deployment failure on the hardware.
    struct stat existing {};
    if (::lstat(destination, &existing) == 0) {
        errno = EEXIST;
        return -1;
    }
    return __real_rename(source, destination);
}

int main(int argc, char** argv) {
    if (argc != 3)
        return 2;
    const std::string root = argv[1];
    const std::string target = root + "/WiiCompiled-Switch.nro";
    mkw::debug::HostConfig config;
    config.root = root.c_str();
    config.nro_path = target.c_str();
    config.port = 0;
    if (std::string(argv[2]) == "enabled")
        config.enabled = true;
    if (!mkw::debug::start(config) && config.enabled)
        return 3;
    mkw::debug::note_stage("host-contract");
    mkw::debug::note_dispatch(608214, 0x8019B964, "NANDSeek");
    mkw::debug::note_present();
    std::cout << "PORT " << mkw::debug::bound_port() << std::endl;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "STOP")
            break;
        if (line == "FAIL_COMMIT")
            rename_fault.store(1);
        if (line == "CRASH_COMMIT")
            rename_fault.store(2);
        if (line.starts_with("LOG ")) {
            const std::string payload = line.substr(4) + "\n";
            mkw::debug::log(payload.data(), payload.size());
        }
        if (line == "FLOOD") {
            const std::string payload(4096, 'x');
            for (int i = 0; i < 16384; ++i) {
                mkw::debug::log(payload.data(), payload.size());
            }
        }
        std::cout << "DONE" << std::endl;
    }
    mkw::debug::stop();
    std::cout << "STOPPED" << std::endl;
}
